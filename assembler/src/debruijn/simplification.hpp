/*
 * simplification.hpp
 *
 *  Created on: 1 Sep 2011
 *      Author: valery
 */

#pragma once

#include "standard.hpp"
#include "construction.hpp"
#include "omni_labelers.hpp"

namespace debruijn_graph {
void simplify_graph(PairedReadStream& stream, conj_graph_pack& gp,
		paired_info_index& paired_index);
} // debruijn_graph

// move impl to *.cpp
namespace debruijn_graph {

template<size_t k>
void PrintWeightDistribution(Graph &g, const string &file_name) {
	ofstream os(file_name.c_str());
	for(auto it = g.SmartEdgeBegin(); !it.IsEnd(); ++it) {
		vector<EdgeId> v1 = g.OutgoingEdges(g.EdgeStart(*it));
		vector<EdgeId> v2 = g.IncomingEdges(g.EdgeEnd(*it));
		bool eq = false;
		if(v1.size() == 2 && v2.size() == 2)
			if((v1[0] == v2[0] && v1[1] == v2[1]) || (v1[0] == v2[1] && v1[0] == v2[1]))
				eq = false;
		if(g.length(*it) > k - 10 && g.length(*it) <= k + 1 && g.OutgoingEdgeCount(g.EdgeStart(*it))>= 2 && g.IncomingEdgeCount(g.EdgeEnd(*it))>= 2 && !eq)
			os << g.coverage(*it) << endl;
	}
	os.close();
}

void simplify_graph(PairedReadStream& stream, conj_graph_pack& gp,
		paired_info_index& paired_index) {
	using namespace omnigraph;

	total_labeler_graph_struct graph_struct(gp.g, &gp.int_ids, &gp.edge_pos);
	total_labeler tot_lab(&graph_struct);

	exec_construction(stream, gp, tot_lab, paired_index);
	INFO("STAGE == Simplifying graph");

	EdgeQuality<Graph> quality_labeler(gp.g, gp.index, gp.kmer_mapper, gp.genome);

//	PrintWeightDistribution<K>(gp.g, "distribution.txt");
	SimplifyGraph<K>(gp, quality_labeler, tot_lab, 10,
			cfg::get().output_dir/*, etalon_paired_index*/);

	//  ProduceInfo<k>(g, index, *totLab, genome, output_folder + "simplified_graph.dot", "simplified_graph");

	//experimental
//	if (cfg::get().paired_mode) {
//		INFO("Pair info aware ErroneousConnectionsRemoval");
//		RemoveEroneousEdgesUsingPairedInfo(gp.g, paired_index);
//		INFO("Pair info aware ErroneousConnectionsRemoval stats");
//		CountStats<K>(gp.g, gp.index, gp.genome);
//	}
	//experimental

	//	ProduceDetailedInfo<k>(g, index, labeler, genome, output_folder + "with_pair_info_edges_removed/",	"graph.dot", "no_erroneous_edges_graph");

	//  WriteGraphComponents<k>(g, index, *totLab, genome, output_folder + "graph_components" + "/", "graph.dot",
	//            "graph_component", cfg::get().ds.IS);

	//  number_of_components = PrintGraphComponents(output_folder + "graph_components/graph", g,
	//            cfg::get().ds.IS, int_ids, paired_index, EdgePos);
}

void load_simplification(conj_graph_pack& gp, paired_info_index& paired_index,
		files_t* used_files) {
	fs::path p = fs::path(cfg::get().load_from) / "simplified_graph";
	used_files->push_back(p);

	// TODO: what's the difference with construction?
	ScanWithPairedIndex(p.string(), gp, paired_index);
}

void save_simplification(conj_graph_pack& gp, paired_info_index& paired_index) {
	fs::path p = fs::path(cfg::get().output_saves) / "simplified_graph";

	PrintWithPairedIndex(p.string(), gp, paired_index);

	//todo temporary solution!!!
	OutputContigs(gp.g, cfg::get().additional_contigs);
	OutputContigs(gp.g, cfg::get().output_dir + "final_contigs.fasta");

// run script automatically takes simplified contigs from correct path

//	OutputContigs(gp.g,
//			cfg::get().output_root + "../" + cfg::get().additional_contigs);
}

void exec_simplification(PairedReadStream& stream, conj_graph_pack& gp, paired_info_index& paired_index) {
	if (cfg::get().entry_point <= ws_simplification) {

		simplify_graph(stream, gp, paired_index);
		save_simplification(gp, paired_index);
	} else {
		INFO("Loading Simplification");

		files_t used_files;
		load_simplification(gp, paired_index, &used_files);
		copy_files_by_prefix(used_files, cfg::get().output_saves);
	}
}

} //debruijn_graph
