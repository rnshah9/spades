//***************************************************************************
//* Copyright (c) 2011-2013 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

/*
 * graph_construction.hpp
 *
 *  Created on: Aug 12, 2011
 *      Author: sergey
 */

#ifndef GRAPH_CONSTRUCTION_HPP_
#define GRAPH_CONSTRUCTION_HPP_

#include "openmp_wrapper.h"

#include "io/multifile_reader.hpp"
#include "omni/edges_position_handler.hpp"

#include "debruijn_graph_constructor.hpp"
#include "indices/debruijn_kmer_index.hpp"
#include "debruijn_graph.hpp"
#include "graphio.hpp"
#include "graph_pack.hpp"
#include "utils.hpp"
#include "perfcounter.hpp"
#include "early_simplification.hpp"

#include "read_converter.hpp"

namespace debruijn_graph {

typedef io::IReader<io::SingleRead> SingleReadStream;
typedef io::IReader<io::PairedRead> PairedReadStream;
typedef io::MultifileReader<io::SingleRead> CompositeSingleReadStream;
typedef io::ConvertingReaderWrapper UnitedStream;

template<class PairedRead>
void FillPairedIndexWithReadCountMetric(const Graph &g,
		const IdTrackHandler<Graph>& int_ids, const EdgeIndex<Graph>& index,
		const KmerMapper<Graph>& kmer_mapper,
		PairedInfoIndexT<Graph>& paired_info_index,
		io::ReadStreamVector<io::IReader<PairedRead> >& streams, size_t k) {

	INFO("Counting paired info with read count weight");
	NewExtendedSequenceMapper<Graph> mapper(g, index, kmer_mapper, k + 1);
	LatePairedIndexFiller<Graph, NewExtendedSequenceMapper<Graph>,
			io::IReader<PairedRead> > pif(g, mapper, streams,
			PairedReadCountWeight);

//	ExtendedSequenceMapper<k + 1, Graph> mapper(g, int_ids, index, kmer_mapper);
//	LatePairedIndexFiller<k + 1, Graph, ExtendedSequenceMapper<k + 1, Graph>, ReadStream> pif(g, mapper, stream, PairedReadCountWeight);

	pif.FillIndex(paired_info_index);
	DEBUG("Paired info with read count weight counted");
}

template<class PairedRead>
void FillPairedIndexWithProductMetric(const Graph &g,
		const EdgeIndex<Graph>& index, const KmerMapper<Graph>& kmer_mapper,
		PairedInfoIndexT<Graph>& paired_info_index,
		io::ReadStreamVector<io::IReader<PairedRead> >& streams, size_t k) {

	INFO("Counting paired info with product weight");

	//	ExtendedSequenceMapper<k + 1, Graph> mapper(g, int_ids, index, kmer_mapper);
	//	LatePairedIndexFiller<k + 1, Graph, ExtendedSequenceMapper<k + 1, Graph>, ReadStream> pif(g, mapper, stream, PairedReadCountWeight);

	NewExtendedSequenceMapper<Graph> mapper(g, index, kmer_mapper, k + 1);
	LatePairedIndexFiller<Graph, NewExtendedSequenceMapper<Graph>,
			io::IReader<PairedRead> > pif(g, mapper, streams,
			KmerCountProductWeight);
	pif.FillIndex(paired_info_index);
	DEBUG("Paired info with product weight counted");
}

void FillEtalonPairedIndex(PairedInfoIndexT<Graph>& etalon_paired_index,
		const Graph &g, const EdgeIndex<Graph>& index,
		const KmerMapper<Graph>& kmer_mapper, size_t is, size_t rs,
		size_t delta, const Sequence& genome, size_t k)
{
	VERIFY_MSG(genome.size() > 0,
			"The genome seems not to be loaded, program will exit");
	INFO((string) (FormattedString("Counting etalon paired info for genome of length=%i, k=%i, is=%i, rs=%i, delta=%i") << genome.size() << k << is << rs << delta));

	EtalonPairedInfoCounter<Graph> etalon_paired_info_counter(g, index, kmer_mapper, is, rs, delta, k);
	etalon_paired_info_counter.FillEtalonPairedInfo(genome, etalon_paired_index);

	DEBUG("Etalon paired info counted");
}

void FillEtalonPairedIndex(PairedInfoIndexT<Graph>& etalon_paired_index,
		const Graph &g, const EdgeIndex<Graph>& index,
		const KmerMapper<Graph>& kmer_mapper, const Sequence& genome,
		size_t k) {

  const auto& ds = cfg::get().ds;
	FillEtalonPairedIndex(etalon_paired_index, g, index, kmer_mapper,
                        ds.IS(), ds.RL(), size_t(ds.is_var()),
			genome, k);
	//////////////////DEBUG
	//	SimpleSequenceMapper<k + 1, Graph> simple_mapper(g, index);
	//	Path<EdgeId> path = simple_mapper.MapSequence(genome);
	//	SequenceBuilder sequence_builder;
	//	sequence_builder.append(Seq<k>(g.EdgeNucls(path[0])));
	//	for (auto it = path.begin(); it != path.end(); ++it) {
	//		sequence_builder.append(g.EdgeNucls(*it).Subseq(k));
	//	}
	//	Sequence new_genome = sequence_builder.BuildSequence();
	//	NewEtalonPairedInfoCounter<k, Graph> new_etalon_paired_info_counter(g, index,
	//			insert_size, read_length, insert_size * 0.1);
	//	PairedInfoIndexT<Graph> new_paired_info_index(g);
	//	new_etalon_paired_info_counter.FillEtalonPairedInfo(new_genome, new_paired_info_index);
	//	CheckInfoEquality(etalon_paired_index, new_paired_info_index);
	//////////////////DEBUG
//	INFO("Etalon paired info counted");
}

void FillCoverageFromIndex(Graph& /*g*/, EdgeIndex<Graph>& index, size_t /*k*/) {
	EdgeIndex<Graph>::InnerIndex &innerIndex = index.inner_index();

	for (auto I = innerIndex.value_cbegin(), E = innerIndex.value_cend();
			I != E; ++I) {
		const auto& edgeInfo = *I;
		VERIFY(edgeInfo.edgeId_.get() != NULL);
		edgeInfo.edgeId_->IncCoverage(edgeInfo.count_);
	}

	DEBUG("Coverage counted");
}

template<class Graph, class Readers, class Seq>
size_t ConstructGraphUsingOldIndex(size_t k,
		Readers& streams, Graph& g,
		EdgeIndex<Graph, Seq>& index, SingleReadStream* contigs_stream = 0) {
	INFO("Constructing DeBruijn graph");

	TRACE("Filling indices");
	size_t rl = 0;
	VERIFY_MSG(streams.size(), "No input streams specified");

	TRACE("... in parallel");
	DeBruijnEdgeIndex<Graph, Seq>& debruijn = index.inner_index();
	rl = DeBruijnEdgeIndexBuilder<Seq>().BuildIndexFromStream(debruijn, streams,
                                                            contigs_stream);

	VERIFY(k + 1== debruijn.K());
	// FIXME: output_dir here is damn ugly!

	TRACE("Filled indices");

	INFO("Condensing graph");
	DeBruijnGraphConstructor<Graph, Seq> g_c(g, debruijn, k);
  TRACE("Constructor ok");
	g_c.ConstructGraph(100, 10000, 1.2); // TODO: move magic constants to config
	TRACE("Graph condensed");

	return rl;
}

template<class Graph, class Read, class Seq>
size_t ConstructGraphUsingExtentionIndex(size_t k, const debruijn_config::construction params,
		io::ReadStreamVector<io::IReader<Read> >& streams, Graph& g,
		EdgeIndex<Graph, Seq>& index, SingleReadStream* contigs_stream = 0) {

	INFO("Constructing DeBruijn graph");

	TRACE("Filling indices");
	VERIFY_MSG(streams.size(), "No input streams specified");

	TRACE("... in parallel");
	// FIXME: output_dir here is damn ugly!
	DeBruijnExtensionIndex<Seq> ext(k, index.inner_index().workdir());
	size_t rl = DeBruijnExtensionIndexBuilder<Seq>().BuildIndexFromStream(ext, streams, contigs_stream);

	TRACE("Extention Index constructed");

    if (params.early_tc.enable) {
        size_t length_bound = rl - k;
        if (params.early_tc.length_bound)
            length_bound = params.early_tc.length_bound.get();
        EarlyTipClipper(ext, length_bound).ClipTips();
    }

	INFO("Condensing graph");
	index.Detach();
	DeBruijnGraphExtentionConstructor<Graph, Seq> g_c(g, ext, k);
	g_c.ConstructGraph(100, 10000, 1.2, params.keep_perfect_loops);//TODO move these parameters to config
	index.Attach();
	INFO("Graph condensed");

	INFO("Counting coverage");
	auto& debruijn = index.inner_index();
	DeBruijnEdgeIndexBuilder<Seq>().BuildIndexWithCoverageFromGraph(g, debruijn, streams, contigs_stream);
	INFO("Counting coverage finished");
	return rl;
}

template<class Graph, class Read, class Seq>
size_t ConstructGraph(size_t k, const debruijn_config::construction &params,
		io::ReadStreamVector<io::IReader<Read> >& streams, Graph& g,
		EdgeIndex<Graph, Seq>& index, SingleReadStream* contigs_stream = 0) {
	if(params.con_mode == construction_mode::con_extention) {
		return ConstructGraphUsingExtentionIndex(k, params, streams, g, index, contigs_stream);
	} else if(params.con_mode == construction_mode::con_old){
		return ConstructGraphUsingOldIndex(k, streams, g, index, contigs_stream);
	} else {
		INFO("Invalid construction mode")
		VERIFY(false);
		return 0;
	}
}

template<class Read>
size_t ConstructGraphWithCoverage(size_t k, const debruijn_config::construction &params,
		io::ReadStreamVector<io::IReader<Read> >& streams, Graph& g,
		EdgeIndex<Graph>& index, SingleReadStream* contigs_stream = 0) {
	size_t rl = ConstructGraph(k, params, streams, g, index, contigs_stream);

	FillCoverageFromIndex(g, index, k);


	return rl;
}

template<class Graph, class Reader, class Index>
size_t ConstructGraphFromStream(size_t k, const debruijn_config::construction params,
        Reader& stream, Graph& g,
        Index& index, SingleReadStream* contigs_stream = 0) {
    io::ReadStreamVector<io::IReader<typename Reader::read_type>> streams(stream);
    return ConstructGraph(k, params, streams, g, index, contigs_stream);
}

template<class Graph, class Reader, class Index>
size_t ConstructGraphWithCoverageFromStream(size_t k,
        Reader& stream, Graph& g,
        Index& index, SingleReadStream* contigs_stream = 0) {
    io::ReadStreamVector<io::IReader<typename Reader::read_type>> streams(stream);
    return ConstructGraph(k, streams, g, index, contigs_stream);
}

}

#endif /* GRAPH_CONSTRUCTION_HPP_ */
