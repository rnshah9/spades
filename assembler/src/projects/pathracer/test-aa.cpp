#include <gtest/gtest.h>
#include "graph.hpp"
#include "aa_cursor.hpp"

// #include <iostream>

template <class GraphCursor>
std::string traverse(GraphCursor cursor) {
  if (cursor.is_empty()) {
    return "";
  }

  std::string result;

  while (true) {
    result += cursor.letter(nullptr);
    auto nexts = cursor.next(nullptr);
    if (nexts.empty()) {
      break;
    }
    cursor = nexts[0];
  }

  return result;
}

std::string translate_via_cursor(const std::string &nts) {
  auto nt_graph = Graph({nts});
  auto aa_cursor = make_aa_cursors(nt_graph.begins(), nullptr)[0];
  return traverse(aa_cursor);
}

TEST(TranslateAntibody, TRANSLATION) {
  std::string nts = "GAGGTGCAGCTGGTGGAGTCTGGGGGAGGTGTGGTACGGCCTGGGGGGTCCCTGAGACTCTCCTGTGCAGCCTCTGGATTCACCTTTGATGATTATGGCATGAGCTGGGTCCGCCAAGCTCCAGGGAAGGGGCTGGAGTGGGTCTCTGGTATTAATTGGAATGGTGGTAGCACAGGTTATGCAGACTCTGTGAAGGGCCGATTCACCATCTCCAGAGACAACGCCAAGAACTCCCTGTATCTGCAAATGAACAGTCTGAGAGCCGAGGACACGGCCTTGTATCACTGTGCGAGAGATCATGATAGTAGTAGCCCGGGGTCCAACTGGTTCGACCCCTGGGGCCAGGGAACCCTGGTCACC";
  std::string aas = "EVQLVESGGGVVRPGGSLRLSCAASGFTFDDYGMSWVRQAPGKGLEWVSGINWNGGSTGYADSVKGRFTISRDNAKNSLYLQMNSLRAEDTALYHCARDHDSSSPGSNWFDPWGQGTLVT";
  EXPECT_EQ(aa::translate(nts), aas);
}

TEST(TranslateViaCursorAntibody, TRANSLATION) {
  std::string nts = "GAGGTGCAGCTGGTGGAGTCTGGGGGAGGTGTGGTACGGCCTGGGGGGTCCCTGAGACTCTCCTGTGCAGCCTCTGGATTCACCTTTGATGATTATGGCATGAGCTGGGTCCGCCAAGCTCCAGGGAAGGGGCTGGAGTGGGTCTCTGGTATTAATTGGAATGGTGGTAGCACAGGTTATGCAGACTCTGTGAAGGGCCGATTCACCATCTCCAGAGACAACGCCAAGAACTCCCTGTATCTGCAAATGAACAGTCTGAGAGCCGAGGACACGGCCTTGTATCACTGTGCGAGAGATCATGATAGTAGTAGCCCGGGGTCCAACTGGTTCGACCCCTGGGGCCAGGGAACCCTGGTCACC";
  EXPECT_EQ(translate_via_cursor(nts), aa::translate(nts));
}

TEST(StopCodons, TRANSLATION) {
  std::string nts = "TAGTAATGA";
  std::string aas = "XXX";
  EXPECT_EQ(aa::translate(nts), aas);
}

// int main() {
//   std::string s = "GAGGTGCAGCTGGTGGAGTCTGGGGGAGGTGTGGTACGGCCTGGGGGGTCCCTGAGACTCTCCTGTGCAGCCTCTGGATTCACCTTTGATGATTATGGCATGAGCTGGGTCCGCCAAGCTCCAGGGAAGGGGCTGGAGTGGGTCTCTGGTATTAATTGGAATGGTGGTAGCACAGGTTATGCAGACTCTGTGAAGGGCCGATTCACCATCTCCAGAGACAACGCCAAGAACTCCCTGTATCTGCAAATGAACAGTCTGAGAGCCGAGGACACGGCCTTGTATCACTGTGCGAGAGATCATGATAGTAGTAGCCCGGGGTCCAACTGGTTCGACCCCTGGGGCCAGGGAACCCTGGTCACC";
//   std::string translated = "EVQLVESGGGVVRPGGSLRLSCAASGFTFDDYGMSWVRQAPGKGLEWVSGINWNGGSTGYADSVKGRFTISRDNAKNSLYLQMNSLRAEDTALYHCARDHDSSSPGSNWFDPWGQGTLVT";
//   auto graph = Graph({s});
//
//   auto aa_cursor = make_aa_cursors(graph.begins())[0];
//
//   std::cout << traverse(aa_cursor) << std::endl;
//
//   return 0;
// }

// vim: set ts=2 sw=2 et :
