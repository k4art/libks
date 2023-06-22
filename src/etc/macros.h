#define KS_CONCAT_(a_, b_)  a_##b_
#define KS_CONCAT(a_, b_)   KS_CONCAT_(a_, b_)
#define KS_MACRO_VAR(name_) KS_CONCAT(name_, __LINE__)

#define KS_DEFER(start_, end_)              \
  for (int KS_MACRO_VAR(_i_) = (start_, 0); \
       !KS_MACRO_VAR(_i_);                  \
       (KS_MACRO_VAR(_i_) += 1, end_))

#define KS_CRITICAL_SECTION(p_mutex_)    \
  KS_DEFER(pthread_mutex_lock(p_mutex_), \
           pthread_mutex_unlock(p_mutex_))

#define KS__TYPE_OF_OR_EXTENDED(expr_, type_, field_)        \
  typeof(_Generic((expr_),                                   \
                  type_   : *(struct { type_ field_; } *) 0, \
                  default : (expr_)))

#define KS__IS_TYPE_OF_OR_EXTENDED(expr_, type_, field_) \
  (offsetof(KS__TYPE_OF_OR_EXTENDED((expr_), type_, field_), field_) == 0)

#define KS_STATIC_ASSERT_TYPE_OR_EXTENDED(expr_, type_, field_)              \
  static_assert(KS__IS_TYPE_OF_OR_EXTENDED((expr_), type_, field_),          \
                "Expression (" #expr_ ") is expected to extend type " #type_ \
                " via field " #field_)

#define KS_STATIC_ASSERT_TYPE(expr_, type_)                \
  static_assert(_Generic((expr_), type_ : 1, default : 0), \
                "Expression (" #expr_ ") is expected to be of type " #type_)

#define KS_STATIC_ASSERT_TYPE_CONST(expr_, type_)                           \
  static_assert(_Generic((expr_), const type_ : 1, type_ : 1, default : 0), \
                "Expression (" #expr_ ") is expected to be of type " #type_)

#define KS_MIN(a, b)                                             \
  ({                                                             \
    __typeof__(a) a_val = (a);                                   \
    __typeof__(b) b_val = (b);                                   \
    a_val < b_val ? a_val : b_val;                               \
  })
