#include "precomp.h"

#include "lib_algorithms.h"
#include "lib_models.h"

#include "dte_model_decorator.h"

namespace lib_models {
template <typename T>
DteModelDecorator<T>::DteModelDecorator() {}

template <typename T>
void DteModelDecorator<T>::AggregateModels(
    col_array<sp<lib_models::MlModel>> models) {
  if (models.size() < 2) return;

  col_array<T> aggregate_prob;
  std::function<void(int)> rec_add_nodes;
  col_array<lib_algorithms::DteAlgorithmShared::Dte_NodeHeader_Classify<T>>
      aggregate_node;

  for (int i = 0; i < models.size(); ++i) {
    auto& node_headers =
        models[i]
            ->Get<col_array<lib_algorithms::DteAlgorithmShared::
                                Dte_NodeHeader_Classify<T>>>(
                ModelsLib::kNodeArray);
    auto& prob_data = models[i]->Get<col_array<T>>(ModelsLib::kProbArray);
    auto trees = models[i]->Get<int>(ModelsLib::kNrTrees);
    auto targets = models[i]->Get<int>(ModelsLib::kNrTargets);

    aggregate_node.reserve(aggregate_node.size() + trees);
    for (int ii = 0; ii < trees; ++ii) {
      aggregate_node.emplace_back(node_headers[ii]);
      if (aggregate_node.back().child_count <= 0) {
        for (int iii = 0; iii < targets; ++iii)
          aggregate_prob.emplace_back(
              prob_data[node_headers[ii].probability_start + iii]);
      }
    }
  }

  auto trees_agg = 0;
  for (int i = 0; i < models.size(); ++i) {
    auto& node_headers =
        models[i]
            ->Get<col_array<lib_algorithms::DteAlgorithmShared::
                                Dte_NodeHeader_Classify<T>>>(
                ModelsLib::kNodeArray);
    auto& prob_data = models[i]->Get<col_array<T>>(ModelsLib::kProbArray);
    auto trees = models[i]->Get<int>(ModelsLib::kNrTrees);
    auto targets = models[i]->Get<int>(ModelsLib::kNrTargets);
    rec_add_nodes = [&](int node_id) {
      if (aggregate_node[node_id].child_count > 0) {
        int child_start = int(aggregate_node.size());
        for (int i = 0; i < aggregate_node[node_id].child_count; ++i) {
          aggregate_node.emplace_back(
              node_headers[aggregate_node[node_id].child_start + i]);
        }
        aggregate_node[node_id].child_start = child_start;
        for (int i = 0; i < aggregate_node[node_id].child_count; ++i)
          rec_add_nodes(child_start + i);
      } else {
        int prob_start = int(aggregate_prob.size());
        for (int ii = 0; ii < targets; ++ii)
          aggregate_prob.emplace_back(
              prob_data[aggregate_node[node_id].probability_start + ii]);
        aggregate_node[node_id].probability_start = prob_start;
      }
    };

    aggregate_node.reserve(aggregate_node.size() + node_headers.size() - trees);
    aggregate_prob.reserve(aggregate_prob.size() + prob_data.size());

    for (int ii = 0; ii < trees; ++ii) rec_add_nodes(trees_agg + ii);
    trees_agg += trees;

    node_headers.clear();
    prob_data.clear();
    node_headers.shrink_to_fit();
    prob_data.shrink_to_fit();
  }

  aggregate_node.shrink_to_fit();
  aggregate_prob.shrink_to_fit();
  models[0]->Add(ModelsLib::kNrTrees, trees_agg);
  models[0]->Add(ModelsLib::kNodeArray, aggregate_node);
  models[0]->Add(ModelsLib::kProbArray, aggregate_prob);
}

template <typename T>
col_array<sp<lib_models::MlModel>> DteModelDecorator<T>::SplitModel(
    sp<lib_models::MlModel> model, const int parts) {
  auto trees = model->Get<int>(ModelsLib::kNrTrees);
  auto targets = model->Get<int>(ModelsLib::kNrTargets);
  auto decorator = ModelsLib::GetInstance().CreateDteModelDecorator<T>();
  col_array<sp<lib_models::MlModel>> models(
      parts, ModelsLib::GetInstance().CreateModel(decorator));
  auto& node_headers = model->Get<col_array<
      lib_algorithms::DteAlgorithmShared::Dte_NodeHeader_Classify<T>>>(
      ModelsLib::kNodeArray);
  auto& prob_data = model->Get<col_array<T>>(ModelsLib::kProbArray);

  std::function<void(int)> rec_add_nodes;
  auto tree_split = trees / parts;
  for (int i = 0; i < parts; ++i) {
    col_array<T> prob_array;
    col_array<lib_algorithms::DteAlgorithmShared::Dte_NodeHeader_Classify<T>>
        node_array;

    rec_add_nodes = [&](int node_id) {
      if (node_array[node_id].child_count > 0) {
        int child_start = int(node_array.size());
        for (int i = 0; i < node_array[node_id].child_count; ++i) {
          node_array.emplace_back(
              node_headers[node_array[node_id].child_start + i]);
        }
        node_array[node_id].child_start = child_start;
        for (int i = 0; i < node_array[node_id].child_count; ++i)
          rec_add_nodes(child_start + i);
      } else {
        int prob_start = int(prob_array.size());
        for (int ii = 0; ii < targets; ++ii)
          prob_array.emplace_back(
              prob_data[node_array[node_id].probability_start + ii]);
        node_array[node_id].probability_start = prob_start;
      }
    };

    models[i]->Add(ModelsLib::kNrTrees, tree_split);
    models[i]->Add(ModelsLib::kNrTargets, targets);
    models[i]->Add(ModelsLib::kNrFeatures,
                   model->Get<int>(ModelsLib::kNrFeatures));
    models[i]->Add(
        ModelsLib::kModelType,
        model->Get<AlgorithmsLib::AlgorithmType>(ModelsLib::kModelType));

    auto tree_offset = tree_split * i;
    for (int ii = 0; ii < tree_split; ++ii) {
      node_array.emplace_back(node_headers[tree_offset + ii]);
      if (node_array.back().child_count <= 0) {
        int prob_start = int(prob_array.size());
        for (int iii = 0; iii < targets; ++iii)
          prob_array.emplace_back(
              prob_data[node_headers[tree_offset + ii].probability_start +
                        iii]);
        node_array.back().probability_start = prob_start;
      }
    }

    for (int ii = 0; ii < tree_split; ++ii) rec_add_nodes(ii);

    models[i]->Add(ModelsLib::kNodeArray, node_array);
    models[i]->Add(ModelsLib::kProbArray, prob_array);
  }
  return models;
}

template <typename T>
void DteModelDecorator<T>::SaveModel(string save_path,
                                     sp<lib_models::MlModel> model) {
  auto& node_array = model->Get<col_array<
      lib_algorithms::DteAlgorithmShared::Dte_NodeHeader_Classify<T>>>(
      ModelsLib::kNodeArray);
  auto& prob_array = model->Get<col_array<T>>(ModelsLib::kProbArray);
  auto nr_trees = model->Get<int>(ModelsLib::kNrTrees);
  auto nr_targets = model->Get<int>(ModelsLib::kNrTargets);
  auto nr_features = model->Get<int>(ModelsLib::kNrFeatures);
  auto model_type = model->Get<AlgorithmsLib::AlgorithmType>(ModelsLib::kModelType);

  std::ofstream open(save_path, std::ios::binary);
  open.write((char*)&nr_trees, sizeof(nr_trees));
  open.write((char*)&nr_targets, sizeof(nr_targets));
  open.write((char*)&nr_features, sizeof(nr_features));
  open.write((char*)&model_type, sizeof(model_type));
  auto node_size = node_array.size();
  open.write((char*)&node_size, sizeof(node_size));
  if (node_size > 0) open.write((char*)node_array.data(), sizeof(node_array[0]));
  auto prob_size = prob_array.size();
  open.write((char*)&prob_size, sizeof(prob_size));
  if (prob_size > 0) open.write((char*)prob_array.data(), sizeof(prob_array[0]));
  open.close();
}

template <typename T>
void DteModelDecorator<T>::LoadModel(string model_path,
                                     sp<lib_models::MlModel> model) {
  int nr_trees = 0;
  int nr_targets = 0;
  int nr_features = 0;
  AlgorithmsLib::AlgorithmType model_type;
  col_array<lib_algorithms::DteAlgorithmShared::Dte_NodeHeader_Classify<T>>
      node_array;
  col_array<T> prob_array;

  std::ifstream open(model_path, std::ios::binary);
  open.read((char*)&nr_trees, sizeof(nr_trees));
  open.read((char*)&nr_targets, sizeof(nr_targets));
  open.read((char*)&nr_features, sizeof(nr_features));
  open.read((char*)&model_type, sizeof(model_type));
  auto node_size = 0;
  open.read((char*)&node_size, sizeof(node_size));
  node_array.resize(node_size);
  if (node_size > 0) open.read((char*)node_array.data(), sizeof(node_array[0]));
  auto prob_size = 0;
  open.read((char*)&prob_size, sizeof(prob_size));
  prob_array.resize(prob_size);
  if (prob_size > 0) open.read((char*)prob_array.data(), sizeof(prob_array[0]));
  open.close();
}

template DteModelDecorator<float>::DteModelDecorator();
template DteModelDecorator<double>::DteModelDecorator();
}