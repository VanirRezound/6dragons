#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value) \
  if(!str_cmp(word, literal)) \
  { \
    field = value; \
    fMatch = TRUE; \
   break; \
   }
