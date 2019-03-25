//#define DEBUG_WOLFSSL

#define HAVE_LIBZ
#define HAVE_ECC
#define HAVE_AESGCM
//#define HAVE_FIPS
#define HAVE_TLS_EXTENSIONS
#define HAVE_SNI
#define HAVE_SUPPORTED_CURVES

#define ECC_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT

#define WOLFSSL_RIPEMD
#define WOLFSSL_SHA512
#define WOLFSSL_SMALL_STACK
// curl‚ÅŽg—p‚µ‚Ä‚¢‚é
#define WOLFSSL_ALLOW_TLSV10

#define WC_RSA_BLINDING

#define SINGLE_THREADED

#define CUSTOM_RAND_GENERATE_SEED custom_rand_generate_seed
