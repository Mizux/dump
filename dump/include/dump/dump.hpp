// DUMP_VARS() is a convenience macro for writing objects to text logs. It
// prints all of its arguments as key-value pairs.
//
// Example:
//
//   int foo = 42;
//   vector<int> bar = {1, 2, 3};
//   // Prints: foo = 42, bar.size() = 3
//   LOG(INFO) << DUMP_VARS(foo, bar.size());
//
// DUMP_VARS() produces high quality human-readable output for most types: builtin
// types, strings, protocol buffers, containers, tuples, smart pointers, Status,
// StatusOr, optional, anything with operator<<, and more. If everything else
// fails, objects are hex-dumped.
//
//                     ====[ Semantics ]====
//
//   LOG(INFO) << DUMP_VARS(expr1, ..., exprN);
//
// Member function as() can be used to override field names.
//
//   LOG(INFO) << DUMP_VARS(expr1, ..., exprN).as("name1", ..., "nameN");
//
//                   ====[ Best Practices ]====
//
// Use DUMP_VARS with LOG and CHECK messages to provide extra context.
//
//   LOG(INFO) << "Request processed: " << DUMP_VARS(request, response);
//
//   CHECK_OK(status) << "RPC request to Frobnicator.Frobnicate failed: "
//                    << DUMP_VARS(FLAGS_remote_service_bns, request);
//
// It also works well with the status macros from //ortools/base/status_macros.h
//
//   return MAKE_ERROR(INVALID_ARGUMENT)
//       << "The number of input and output directories should be the same: "
//       << DUMP_VARS(in_dirs.size(), out_dirs.size(), in_dirs, out_dirs);
//
// DUMP_VARS can print values of any type. Do use it in generic functions and
// macros where the types of the arguments are unknown. Best-effort printing is
// better than none.
//
//   template <class K, class V>
//   const V& FindOrDie(const std::map<K, V>& map, const K& key) {
//     auto it = map.find(key);
//     CHECK(it != map.end()) << "Key not found: " << DUMP_VARS(key, map);
//     return it->second;
//   }
//
// Use the following template for all your messages:
//
//   1. Start with a literal message in plain English. If this message simply
//      restates the content of a CHECK macro, it can be omitted.
//   2. Use a single DUMP_VARS() expression at the end of the message with all
//      relevant variables. Remember that you can use arbitrary expressions as
//      arguments.
//   3. Avoid writing variables other than via DUMP_VARS().
//
// There are several advantages to this style:
//
//   * You don't have to waste mental power on creative intermixing of literal
//     text and variables, quoting and conditional pluralization.
//   * It'll always be clear when one variable ends and another starts. Empty
//     strings, and strings with spaces and quotes in them won't change the
//     meaning of your message.
//   * Regardless of the size of the variables, the primary message will always
//     fit in the log and will be easy to read.
//
//   // BAD. The message will be hard to read if src contains a sentence or
//   // special characters, or is empty or very long.
//   CHECK(CopyFile(src, dst)) << "Can't copy " << src << " to " << dst;
//
//   // GOOD. Easy to write and easy to read when the CHECK triggers.
//   CHECK(CopyFile(src, dst)) << "Can't copy file: " << DUMP_VARS(src, dst);
//
//   // BEST. The message wasn't useful.
//   CHECK(CopyFile(src, dst)) << DUMP_VARS(src, dst);
//
// If arguments to DUMP_VARS() aren't descriptive enough to be understood by
// the readers of the logs, override them with the member function as().
//
//    LOG(INFO) << "Opening: " << DUMP_VARS(it->second.value).as("filename");
//
// By default fields are separated with ", " and keys are separated from values
// with " = ". Different separators can be specified by calling the member
// function sep(field_separator, kv_separator). The second argument is optional.
// If it's not specified, the key-value separator stays unchanged.
//
//    VLOG(3) << "Internal state:\n"
//            << DUMP_VARS(a_, b_, c_, d_, e_, f_, g_, h_, i_).sep("\n", ":=");
//
// The arguments of DUMP_VARS get evaluated during streaming. If the same
// DUMP_VARS instance is streamed several times, the arguments are also
// evaluated several times. This allows you to factor out DUMP_VARS() calls
// that are repeated many times in the same function.
//
//   string Configuration::FindBackend(const string& service) {
//     // this->DebugString() isn't called here yet.
//     auto context = DUMP_VARS(service, this->DebugString());
//     // Calls this->DebugString() on CHECK failure.
//     CHECK(IsKnownService(service)) << context;
//     CHECK(IsAccessAllowed(service)) << context;
//     ...
//   }
//
//               ====[ String Representation ]====
//
// The streaming approach described above is generally recommended, for
// efficiency and readability, but some situations may require a string rather
// than a streamable object. The object returned by DUMP_VARS provides a
// convenient str() method for such cases:
//
//   void RpcMethod(RPC*, const Request& req, Response*, Closure*) {
//     VLOG_LINES(1, DUMP_VARS(req).str());
//   }
//
//                    ====[ Limitations ]====
//
// DUMP_VARS() accepts at most 64 arguments.
//
// All arguments to DUMP_VARS() must be perfect-forwardable. Brace-expressions,
// bit fields, and other esoterics are not supported.
//
//   // Compile error: {42} can't be perfect-forwarded.
//   LOG(INFO) << DUMP_VARS({42});
//
// Arguments with unparenthesized commas confuse and frighten DUMP_VARS,
// leading to compile errors. Either parenthesize problematic arguments or
// explicitly provide names with as() to avoid this problem.
//
//   // Compile error. DUMP_VARS gets confused: it thinks we are passing it two
//   // arguments.
//   LOG(INFO) << DUMP_VARS(pair<int, int>());
//
//   // This works! Note the parentheses around the argument.
//   LOG(INFO) << DUMP_VARS((pair<int, int>()));
//
//   // Also works.
//   LOG(INFO) << DUMP_VARS(pair<int, int>()).as("p");
//
// Values of type const char* are printed as pointers, not as strings. This is
// a safety measure because not all pointers to char are null-terminated
// strings. Construct an absl::string_view if string printing is desired.
//
//   const char* s = "hello";
//   // s = 0x1122334455667788
//   LOG(INFO) << DUMP_VARS(s);
//   // absl::string_view(s) = "hello"
//   LOG(INFO) << DUMP_VARS(absl::string_view(s));
//
// Structured bindings require an extra step to make DUMP_VARS print them. They
// need to be listed as first argument of DUMP_VARS_WITH_BINDINGS:
//
//   for (const auto& [x, y, z] : Foo()) {
//     // Would not compile:
//     // LOG(INFO) << DUMP_VARS(x, *y, f(z), other_var);
//     LOG(INFO) << DUMP_VARS_WITH_BINDINGS((x, y, z), x, *y, f(z), other_var);
//   }

#ifndef DUMP_HPP_
#define DUMP_HPP_

#include <ostream>
#include <sstream>
#include <vector>
#include <string>
#include <type_traits>
#include <tuple>
#include <utility>

//#include "absl/base/thread_annotations.h"
//#include "absl/strings/str_cat.h"
//#include "ortools/base/macros.h"

// Returns an ostreamable type that prints all passed arguments as key-value
// pairs. Primarily used for logging.
//
//   int foo = 42;
//   vector<int> bar = {1, 2, 3};
//   LOG(INFO) << DUMP_VARS(foo, bar.size());
//
// Prints:
//
//   foo = 42, bar.size() = 3
//
// NOTE: if you get a compile error saying "no matching function for call to
// 'make_fields'", you need to surround some of the arguments with parentheses.
//
//   // error: no matching function for call to 'make_fields'.
//   // DUMP_VARS gets confused: it thinks we are passing it two arguments.
//   LOG(INFO) << DUMP_VARS(pair<int, int>());
//
//   // This works! Note the parentheses around the argument.
//   LOG(INFO) << DUMP_VARS((pair<int, int>()));
//
// See comments at the top of the file for details.

/* need extra level to force extra eval */
#define DUMP_CONCATENATE(a,b) DUMP_CONCATENATE1(a,b)
#define DUMP_CONCATENATE1(a,b) DUMP_CONCATENATE2(a,b)
#define DUMP_CONCATENATE2(a,b) a ## b

#define DUMP_NARG(...) DUMP_NARG_(__VA_ARGS__,DUMP_RSEQ_N())
#define DUMP_NARG_(...) DUMP_ARG_N(__VA_ARGS__)
#define DUMP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define DUMP_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define DUMP_STRINGIFY_N1(a) #a,
#define DUMP_STRINGIFY_N2(a,b) #a, #b
#define DUMP_STRINGIFY_N3(a,b,c) #a, #b, #c
#define DUMP_STRINGIFY_N4(a,b,c,d) #a, #b, #c, #d
#define DUMP_STRINGIFY_N5(a,b,c,d,e) #a, #b, #c, #d, #e
#define DUMP_STRINGIFY_N6(a,b,c,d,e,f) #a, #b, #c, #d, #e, #f
#define DUMP_STRINGIFY_N7(a,b,c,d,e,f,g) #a, #b, #c, #d, #e, #f, #g
#define DUMP_STRINGIFY_N8(a,b,c,d,e,f,g,h) #a, #b, #c, #d, #e, #f, #g, #h
#define DUMP_STRINGIFY_(M, ...) M(__VA_ARGS__)
#define DUMP_STRINGIFY(...) DUMP_STRINGIFY_(DUMP_CONCATENATE(DUMP_STRINGIFY_N, DUMP_NARG(__VA_ARGS__)), __VA_ARGS__)

#define DUMP(...) DUMP_INTERNAL(ignored, __VA_ARGS__)

#define DUMP_INTERNAL(ignored, ...) \
  ::dump::internal_dump::make_dump<>( \
   ::dump::internal_dump::DumpNames{ DUMP_STRINGIFY(__VA_ARGS__) }, \
   ::std::make_tuple( __VA_ARGS__ ) \
  )


namespace dump {
namespace internal_dump {

using DumpNames = std::vector<std::string>;

template <class... Ts>
class Dump {
 public:
  explicit Dump(
      const std::string&& field_sep,
      const std::string&& kv_sep,
      DumpNames&& names,
      ::std::tuple<Ts...>&& ts) :
        field_sep_(std::move(field_sep)),
        kv_sep_(std::move(kv_sep)),
        names_(std::move(names)),
        ts_(std::move(ts)) {}

  ::std::string str() const {
    ::std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  template <class... T>
  Dump<Ts...> as(T&&... names) const {
    return Dump<Ts...>(
        std::string{field_sep_},
        std::string{kv_sep_},
        DumpNames{names...},
        ::std::tuple<Ts...>{ts_});
  }

  Dump& sep(std::string&& field_sep) {
    field_sep_ = std::move(field_sep);
    return *this;
  }

  Dump& sep(std::string&& field_sep, std::string&& kv_sep) {
    field_sep_ = std::move(field_sep);
    kv_sep_ = std::move(kv_sep);
    return *this;
  }

  // template <class T>
  // Dump& set_writer(T /*new_writer*/) const {
  //   return *this;
  // }

  friend ::std::ostream& operator<<(
      ::std::ostream& os,
      const Dump& dump) {
    dump.print_fields_(os);
    return os;
  }

 private:
  void print_fields_(::std::ostream& os) const {
    std::apply(
        [this, &os](Ts const&... ts) {
          os << '{';
          std::size_t n{0};
          ((os << names_[n] << kv_sep_ << ts << (++n != sizeof...(Ts) ? field_sep_ : "")), ...);
          os << '}';
        }, ts_);
  }

  std::string field_sep_;
  std::string kv_sep_;
  DumpNames names_;
  ::std::tuple<Ts...> ts_;
};

template <class... Ts>
Dump<Ts...> make_dump(DumpNames&& names, ::std::tuple<Ts...>&& ts) {
  return Dump<Ts...>(
      /*field_sep=*/", ",
      /*kv_sep=*/" = ",
      std::move(names),
      std::move(ts));
}

}  // namespace internal_dump
}  // namespace dump

#endif // DUMP_HPP_
