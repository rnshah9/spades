/**
 * @file    reader.hpp
 * @author  Mariya Fomkina
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * @section DESCRIPTION
 *
 * SingleRead is a structure, where information from input files is stored.
 * It includes 3 strings: with id, sequence and quality of the input read.
 */

#ifndef COMMON_IO_READER_HPP_
#define COMMON_IO_READER_HPP_

#include "common/io/single_read.hpp"
#include "common/io/paired_read.hpp"

template<typename ReadType>
class Reader {
 public:
  explicit Reader(const typename ReadType::FilenameType &filename) = 0;
  virtual ~Reader() = 0;
  bool is_open() = 0;
  bool eof() = 0;
  virtual Reader& operator>>(ReadType& read) = 0;
  virtual void close() = 0;
  virtual void reset() = 0;
};

template<>
class Reader<SingleRead> {
 public:
  explicit Reader(const typename SingleRead::FilenameType &filename) {
    // TBD
  }

  virtual ~Reader() {
    close();
  }

  bool is_open() {
    return is_open_;
  }

  bool eof() {
    return eof_;
  }

  virtual Reader& operator>>(SingleRead& singleread) {
    // TBD
  }

  virtual void close() {
    // TBD
  }

  virtual void reset() {
    // TBD
  }

 private:
  typename SingleRead::FilenameType filename_;
  bool is_open_;
  bool eof_;
};

template<>
class Reader<PairedRead> {
 public:
  Reader(const typename SingleRead::FilenameType &filename, size_t distance)
      : distance_(distance), filename_(filename) {
    first_ = new Reader<SingleRead>(filename_.fisrt);
    second_ = new Reader<SingleRead>(filename_.second);
    is_open_ = first_.is_open() && second_.is_open();
    eof_ = first_.eof() || second_.eof();
  }

  virtual ~Reader() {
    close();
  }

  bool is_open() {
    return is_open_;
  }

  bool eof() {
    return eof_;
  }

  virtual Reader& operator>>(PairedRead& pairedread) {
    SingleRead sr1, sr2;
    first_ >> sr1;
    second_ >> sr2;
    pairedread = PairedRead(sr1, sr2, distance_);  // is it correct?
    eof_ = first_.eof() || second_.eof();
    return *this;
  }

  virtual void close() {
    first_.close();
    second_.close();
    is_open_ = false;
  }

  virtual void reset() {
    first_.reset();
    second_.reset();
  }

 private:
  typename SingleRead::FilenameType filename_;
  Reader<SingleRead>* first_;
  Reader<SingleRead>* second_;
  size_t distance_;
  bool is_open_;
  bool eof_;
};

#endif /* COMMON_IO_READER_HPP_ */
