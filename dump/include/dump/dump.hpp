// DUMP() is a convenience macro for writing objects to text logs. It
// prints all of its arguments as key-value pairs.
//
// Example:
//   int foo = 42;
//   vector<int> bar = {1, 2, 3};
//   // Prints: foo = 42, bar.size() = 3
//   LOG(INFO) << DUMP(foo, bar.size());
//
// DUMP() produces high quality human-readable output for most types:
// builtin types, strings, anything with operator<<.
//
//                    ====[ Limitations ]====
//
// DUMP() accepts at most 8 arguments.
//
// Structured bindings require an extra step to make DUMP print them. They
// need to be listed as first argument of DUMP_INTERNAL:
//
//   for (const auto& [x, y, z] : Foo()) {
//     // Would not compile:
//     // LOG(INFO) << DUMP(x, *y, f(z), other_var);
//     LOG(INFO) << DUMP_INTERNAL((x, y, z), x, *y, f(z), other_var);
//   }

#ifndef DUMP_HPP_
#define DUMP_HPP_

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/* need extra level to force extra eval */
//#define DUMP_CONCATENATE(a, b) DUMP_CONCATENATE1(a, b)
//#define DUMP_CONCATENATE1(a, b) DUMP_CONCATENATE2(a, b)
//#define DUMP_CONCATENATE2(a, b) a##b
//
//#define DUMP_EXPAND(x) x
//#define DUMP_NARG(...) DUMP_NARG_(__VA_ARGS__ __VA_OPT__(, ) DUMP_RSEQ_N())
//#define DUMP_NARG_(...) DUMP_EXPAND(DUMP_ARG_N(__VA_ARGS__))
//#define DUMP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
//#define DUMP_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0
//
//#define DUMP_FOR_EACH_N0(F)
//#define DUMP_FOR_EACH_N1(F, a) F(a)
//#define DUMP_FOR_EACH_N2(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N1(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N3(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N2(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N4(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N3(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N5(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N4(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N6(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N5(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N7(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N6(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_N8(F, a, ...) F(a) DUMP_EXPAND(DUMP_FOR_EACH_N7(F, __VA_ARGS__))
//#define DUMP_FOR_EACH_(N, F, ...) DUMP_EXPAND(DUMP_CONCATENATE(DUMP_FOR_EACH_N, N)(F __VA_OPT__(, ) __VA_ARGS__))
//#define DUMP_FOR_EACH(F, ...) DUMP_FOR_EACH_(DUMP_NARG(__VA_ARGS__), F, __VA_ARGS__)

#define DUMP_PARENS ()

#define DUMP_EXPAND(...) DUMP_EXPAND4(DUMP_EXPAND4(DUMP_EXPAND4(DUMP_EXPAND4(__VA_ARGS__))))
#define DUMP_EXPAND4(...) DUMP_EXPAND3(DUMP_EXPAND3(DUMP_EXPAND3(DUMP_EXPAND3(__VA_ARGS__))))
#define DUMP_EXPAND3(...) DUMP_EXPAND2(DUMP_EXPAND2(DUMP_EXPAND2(DUMP_EXPAND2(__VA_ARGS__))))
#define DUMP_EXPAND2(...) DUMP_EXPAND1(DUMP_EXPAND1(DUMP_EXPAND1(DUMP_EXPAND1(__VA_ARGS__))))
#define DUMP_EXPAND1(...) __VA_ARGS__

#define DUMP_FOR_EACH(macro, ...)                                    \
  __VA_OPT__(DUMP_EXPAND(DUMP_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define DUMP_FOR_EACH_HELPER(macro, a1, ...)                         \
  macro(a1)                                                     \
  __VA_OPT__(DUMP_FOR_EACH_AGAIN DUMP_PARENS (macro, __VA_ARGS__))
#define DUMP_FOR_EACH_AGAIN() DUMP_FOR_EACH_HELPER


#define DUMP(...) DUMP_INTERNAL((), __VA_ARGS__)

/* need extra level to force extra eval */
#define DUMP_STRINGIZE(a) #a,
#define DUMP_STRINGIFY(...) DUMP_FOR_EACH(DUMP_STRINGIZE, __VA_ARGS__)

// Returns the arguments.
#define DUMP_IDENTITY(...) __VA_ARGS__
// Removes parenthesis. Requires argument enclosed in parenthesis.
#define DUMP_RM_PARENS(...) DUMP_IDENTITY __VA_ARGS__

#define DUMP_GEN_ONE_BINDING(a) , &a = a
#define DUMP_GEN_BINDING(binding) &DUMP_FOR_EACH(DUMP_GEN_ONE_BINDING, DUMP_RM_PARENS(binding))

#define DUMP_INTERNAL(binding, ...)                    \
  ::dump::internal_dump::make_dump<>(                  \
    ::dump::internal_dump::DumpNames{                  \
          DUMP_STRINGIFY(__VA_ARGS__) },               \
    [DUMP_GEN_BINDING(binding)](                       \
      ::std::ostream& os,                              \
      const ::std::string& field_sep,                  \
      const ::std::string& kv_sep,                     \
      const ::dump::internal_dump::DumpNames& names) { \
      ::dump::internal_dump::print_fields {            \
        .os=os,                                        \
        .field_sep=field_sep,                          \
        .kv_sep=kv_sep,                                \
        .names=names,                                  \
        }(__VA_ARGS__);                                \
      })

namespace dump {
namespace internal_dump {

using DumpNames = ::std::vector<::std::string>;

struct print_fields {
  void operator()() {}

  template <class T>
  void operator()(const T& t) {
    os << names[n++] << kv_sep << t;
  }

  template <class T1, class T2, class... Ts>
  void operator()(const T1& t1, const T2& t2, const Ts&... ts) {
    os << names[n++] << kv_sep << t1 << field_sep;
    (*this)(t2, ts...);
  }

  std::ostream& os;
  const ::std::string& field_sep;
  const ::std::string& kv_sep;
  const DumpNames& names;
  ::std::size_t n = 0;
};

template <class F>
class Dump {
 public:
  explicit Dump(
      const ::std::string&& field_sep,
      const ::std::string&& kv_sep,
      DumpNames&& names,
      F f):
        field_sep_(::std::move(field_sep)),
        kv_sep_(::std::move(kv_sep)),
        names_(::std::move(names)),
        f_(::std::move(f)) {}

  ::std::string str() const {
    ::std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  template <class... N>
  Dump<F> as(N&&... names) const {
    return Dump<F>(
        ::std::string{field_sep_},
        ::std::string{kv_sep_},
        DumpNames{names...},
        f_);
  }

  Dump& sep(::std::string&& field_sep) {
    field_sep_ = ::std::move(field_sep);
    return *this;
  }

  Dump& sep(::std::string&& field_sep, ::std::string&& kv_sep) {
    field_sep_ = ::std::move(field_sep);
    kv_sep_ = ::std::move(kv_sep);
    return *this;
  }

  friend ::std::ostream& operator<<(::std::ostream& os, const Dump& dump) {
    dump.print_fields_(os);
    return os;
  }

 private:
  void print_fields_(::std::ostream& os) const {
    f_(os, field_sep_, kv_sep_, names_);
  }

  ::std::string field_sep_;
  ::std::string kv_sep_;
  DumpNames names_;
  F f_;
};

template <class F>
Dump<F> make_dump(DumpNames&& names, F f) {
  return Dump<F>(
      /*field_sep=*/", ",
      /*kv_sep=*/" = ",
      ::std::move(names),
      ::std::move(f)
  );
}

}  // namespace internal_dump
}  // namespace dump

#endif // DUMP_HPP_
