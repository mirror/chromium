// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_PROTO_MATCHERS_H_
#define COMPONENTS_SYNC_TEST_PROTO_MATCHERS_H_

#include "components/sync/protocol/proto_visitors.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

class ProtoPrinter {
 public:
  template <class P>
  static void Print(const P& proto, ::std::ostream* os) {
    *os << "{\n";
    ProtoPrinter visitor(os, /*indentation=*/1);
    ::syncer::VisitProtoFields(visitor, proto);
    *os << "}";
  }

  ProtoPrinter(std::ostream* os, int indentation)
      : os_(os), indentation_(indentation), indent_str_(indentation * 2, ' ') {}

  template <class P>
  void VisitBytes(const P& parent_proto,
                  const char* field_name,
                  const std::string& field) {
    std::string value;
    for (char c : field) {
      base::StringAppendF(&value, "\\u%04X ", c);
    }
    *os_ << indent_str_ << field_name << ": " << value << "\n";
  }

  // Types derived from MessageLite (i.e. protos).
  template <class P, class F>
  typename std::enable_if<
      std::is_base_of<google::protobuf::MessageLite, F>::value>::type
  Visit(const P&, const char* field_name, const F& field) {
    *os_ << indent_str_ << field_name << " {\n";
    ProtoPrinter visitor(os_, indentation_ + 1);
    ::syncer::VisitProtoFields(visitor, field);
    *os_ << indent_str_ << "}\n";
  }

  // RepeatedPtrField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedPtrField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  // RepeatedField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  template <class P, class E>
  void VisitEnum(const P& parent_proto, const char* field_name, E field) {
    Visit(parent_proto, field_name, field);
  }

  // std::string.
  template <class P>
  void Visit(const P&, const char* field_name, const std::string& field) {
    *os_ << indent_str_ << field_name << ": \"" << field << "\"\n";
  }

  // Default implementation.
  template <class P, class F>
  typename std::enable_if<
      !std::is_base_of<google::protobuf::MessageLite, F>::value>::type
  Visit(const P&, const char* field_name, F field) {
    *os_ << indent_str_ << field_name << ": " << field << "\n";
  }

 private:
  std::ostream* os_;
  const int indentation_;
  const std::string indent_str_;
};

template <typename P>
class EqualsProtoMatcher : public testing::MatcherInterface<P> {
 public:
  explicit EqualsProtoMatcher(const P& expected) : expected_(expected) {}

  bool MatchAndExplain(P actual,
                       testing::MatchResultListener* listener) const override {
    if (expected_.SerializeAsString() == actual.SerializeAsString()) {
      return true;
    }
    if (listener->stream()) {
      *listener << "which is proto ";
      ProtoPrinter::Print<P>(actual, listener->stream());
    }
    return false;
  }

  void DescribeTo(::std::ostream* os) const override {
    *os << "is equal to proto ";
    ProtoPrinter::Print<P>(expected_, os);
  }

  void DescribeNegationTo(::std::ostream* os) const override {
    *os << "is not equal to proto ";
    ProtoPrinter::Print<P>(expected_, os);
  }

 private:
  const P expected_;
};

template <typename P>
testing::Matcher<P> EqualsProto(const P& expected) {
  return testing::MakeMatcher(new EqualsProtoMatcher<P>(expected));
}

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_PROTO_MATCHERS_H_
