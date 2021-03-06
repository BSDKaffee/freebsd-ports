--- plugins/srp.c.orig	2012-10-12 14:05:48 UTC
+++ plugins/srp.c
@@ -89,6 +89,9 @@ typedef unsigned short uint32;
 #include <openssl/hmac.h>
 #include <openssl/md5.h>
 
+/* for legacy libcrypto support */
+#include "crypto-compat.h"
+
 #include <sasl.h>
 #define MD5_H  /* suppress internal MD5 */
 #include <saslplug.h>
@@ -216,22 +219,22 @@ typedef struct srp_options_s {
 typedef struct context {
     int state;
     
-    BIGNUM N;			/* safe prime modulus */
-    BIGNUM g;			/* generator */
+    BIGNUM *N;			/* safe prime modulus */
+    BIGNUM *g;			/* generator */
     
-    BIGNUM v;			/* password verifier */
+    BIGNUM *v;			/* password verifier */
     
-    BIGNUM b;			/* server private key */
-    BIGNUM B;			/* server public key */
+    BIGNUM *b;			/* server private key */
+    BIGNUM *B;			/* server public key */
     
-    BIGNUM a;			/* client private key */
-    BIGNUM A;			/* client public key */
+    BIGNUM *a;			/* client private key */
+    BIGNUM *A;			/* client public key */
     
-    char K[EVP_MAX_MD_SIZE];	/* shared context key */
-    int Klen;
+    unsigned char K[EVP_MAX_MD_SIZE];	/* shared context key */
+    unsigned int Klen;
     
-    char M1[EVP_MAX_MD_SIZE];	/* client evidence */
-    int M1len;
+    unsigned char M1[EVP_MAX_MD_SIZE];	/* client evidence */
+    unsigned int M1len;
     
     char *authid;		/* authentication id (server) */
     char *userid;		/* authorization id (server) */
@@ -242,7 +245,7 @@ typedef struct context {
     char *server_options;
     
     srp_options_t client_opts;	/* cache between client steps */
-    char cIV[SRP_MAXBLOCKSIZE];	/* cache between client steps */
+    unsigned char cIV[SRP_MAXBLOCKSIZE];	/* cache between client steps */
     
     char *salt;			/* password salt */
     int saltlen;
@@ -259,12 +262,12 @@ typedef struct context {
     /* Layer foo */
     unsigned layer;		/* bitmask of enabled layers */
     const EVP_MD *hmac_md;	/* HMAC for integrity */
-    HMAC_CTX hmac_send_ctx;
-    HMAC_CTX hmac_recv_ctx;
+    HMAC_CTX *hmac_send_ctx;
+    HMAC_CTX *hmac_recv_ctx;
 
     const EVP_CIPHER *cipher;	/* cipher for confidentiality */
-    EVP_CIPHER_CTX cipher_enc_ctx;
-    EVP_CIPHER_CTX cipher_dec_ctx;
+    EVP_CIPHER_CTX *cipher_enc_ctx;
+    EVP_CIPHER_CTX *cipher_dec_ctx;
     
     /* replay detection sequence numbers */
     int seqnum_out;
@@ -317,12 +320,12 @@ static int srp_encode(void *context,
 	inputlen = invec[i].iov_len;
     
 	if (text->layer & BIT_CONFIDENTIALITY) {
-	    unsigned enclen;
+	    int enclen;
 
 	    /* encrypt the data into the output buffer */
-	    EVP_EncryptUpdate(&text->cipher_enc_ctx,
-			      text->encode_buf + *outputlen, &enclen,
-			      input, inputlen);
+	    EVP_EncryptUpdate(text->cipher_enc_ctx,
+			      (unsigned char *) text->encode_buf + *outputlen,
+                              &enclen, (unsigned char *) input, inputlen);
 	    *outputlen += enclen;
 
 	    /* switch the input to the encrypted data */
@@ -337,11 +340,12 @@ static int srp_encode(void *context,
     }
     
     if (text->layer & BIT_CONFIDENTIALITY) {
-	unsigned enclen;
+	int enclen;
 
 	/* encrypt the last block of data into the output buffer */
-	EVP_EncryptFinal(&text->cipher_enc_ctx,
-			 text->encode_buf + *outputlen, &enclen);
+	EVP_EncryptFinal(text->cipher_enc_ctx,
+			 (unsigned char *) text->encode_buf + *outputlen,
+                         &enclen);
 	*outputlen += enclen;
     }
 
@@ -349,18 +353,20 @@ static int srp_encode(void *context,
 	unsigned hashlen;
 
 	/* hash the content */
-	HMAC_Update(&text->hmac_send_ctx, text->encode_buf+4, *outputlen-4);
+	HMAC_Update(text->hmac_send_ctx,
+                    (unsigned char *) text->encode_buf+4, *outputlen-4);
 	
 	if (text->layer & BIT_REPLAY_DETECTION) {
 	    /* hash the sequence number */
 	    tmpnum = htonl(text->seqnum_out);
-	    HMAC_Update(&text->hmac_send_ctx, (char *) &tmpnum, 4);
+	    HMAC_Update(text->hmac_send_ctx, (unsigned char *) &tmpnum, 4);
 	    
 	    text->seqnum_out++;
 	}
 
 	/* append the HMAC into the output buffer */
-	HMAC_Final(&text->hmac_send_ctx, text->encode_buf + *outputlen,
+	HMAC_Final(text->hmac_send_ctx,
+                   (unsigned char *) text->encode_buf + *outputlen,
 		   &hashlen);
 	*outputlen += hashlen;
     }
@@ -387,8 +393,8 @@ static int srp_decode_packet(void *conte
 
     if (text->layer & BIT_INTEGRITY) {
 	const char *hash;
-	char myhash[EVP_MAX_MD_SIZE];
-	unsigned hashlen, myhashlen, i;
+	unsigned char myhash[EVP_MAX_MD_SIZE];
+	unsigned hashlen;
 	unsigned long tmpnum;
 
 	hashlen = EVP_MD_size(text->hmac_md);
@@ -405,25 +411,23 @@ static int srp_decode_packet(void *conte
 	hash = input + inputlen;
 
 	/* create our own hash from the input */
-	HMAC_Update(&text->hmac_recv_ctx, input, inputlen);
+	HMAC_Update(text->hmac_recv_ctx, (unsigned char *) input, inputlen);
 	    
 	if (text->layer & BIT_REPLAY_DETECTION) {
 	    /* hash the sequence number */
 	    tmpnum = htonl(text->seqnum_in);
-	    HMAC_Update(&text->hmac_recv_ctx, (char *) &tmpnum, 4);
+	    HMAC_Update(text->hmac_recv_ctx, (unsigned char *) &tmpnum, 4);
 		
 	    text->seqnum_in++;
 	}
 	    
-	HMAC_Final(&text->hmac_recv_ctx, myhash, &myhashlen);
+	HMAC_Final(text->hmac_recv_ctx, myhash, &hashlen);
 
 	/* compare hashes */
-	for (i = 0; i < hashlen; i++) {
-	    if ((myhashlen != hashlen) || (myhash[i] != hash[i])) {
-		SETERROR(text->utils, "Hash is incorrect\n");
-		return SASL_BADMAC;
-	    }
-	}
+        if (memcmp(hash, myhash, hashlen)) {
+            SETERROR(text->utils, "Hash is incorrect\n");
+            return SASL_BADMAC;
+        }
     }
 	
     ret = _plug_buf_alloc(text->utils, &(text->decode_pkt_buf),
@@ -432,16 +436,17 @@ static int srp_decode_packet(void *conte
     if (ret != SASL_OK) return ret;
 	
     if (text->layer & BIT_CONFIDENTIALITY) {
-	unsigned declen;
+	int declen;
 
 	/* decrypt the data into the output buffer */
-	EVP_DecryptUpdate(&text->cipher_dec_ctx,
-			  text->decode_pkt_buf, &declen,
-			  (char *) input, inputlen);
+	EVP_DecryptUpdate(text->cipher_dec_ctx,
+			  (unsigned char *) text->decode_pkt_buf, &declen,
+			  (unsigned char *) input, inputlen);
 	*outputlen = declen;
 	    
-	EVP_DecryptFinal(&text->cipher_dec_ctx,
-			 text->decode_pkt_buf + declen, &declen);
+	EVP_DecryptFinal(text->cipher_dec_ctx,
+			 (unsigned char *) text->decode_pkt_buf + declen,
+                         &declen);
 	*outputlen += declen;
     } else {
 	/* copy the raw input to the output */
@@ -474,7 +479,8 @@ static int srp_decode(void *context,
 /*
  * Convert a big integer to it's byte representation
  */
-static int BigIntToBytes(BIGNUM *num, char *out, int maxoutlen, int *outlen)
+static int BigIntToBytes(BIGNUM *num, unsigned char *out, int maxoutlen,
+                         unsigned int *outlen)
 {
     int len;
     
@@ -504,12 +510,12 @@ static int BigIntCmpWord(BIGNUM *a, BN_U
 /*
  * Generate a random big integer.
  */
-static void GetRandBigInt(BIGNUM *out)
+static void GetRandBigInt(BIGNUM **out)
 {
-    BN_init(out);
+    *out = BN_new();
     
     /* xxx likely should use sasl random funcs */
-    BN_rand(out, SRP_MAXBLOCKSIZE*8, 0, 0);
+    BN_rand(*out, SRP_MAXBLOCKSIZE*8, 0, 0);
 }
 
 #define MAX_BUFFER_LEN 2147483643
@@ -624,7 +630,8 @@ static int MakeBuffer(const sasl_utils_t
 	case 'm':
 	    /* MPI */
 	    mpi = va_arg(ap, BIGNUM *);
-	    r = BigIntToBytes(mpi, out+2, BN_num_bytes(mpi), &len);
+	    r = BigIntToBytes(mpi, (unsigned char *) out+2,
+                              BN_num_bytes(mpi), (unsigned *) &len);
 	    if (r) goto done;
 	    ns = htons(len);
 	    memcpy(out, &ns, 2);	/* add 2 byte len (network order) */
@@ -695,7 +702,7 @@ static int UnBuffer(const sasl_utils_t *
     va_list ap;
     char *p;
     int r = SASL_OK, noalloc;
-    BIGNUM *mpi;
+    BIGNUM **mpi;
     char **os, **str;
     uint32 *u;
     unsigned short ns;
@@ -757,9 +764,12 @@ static int UnBuffer(const sasl_utils_t *
 		goto done;
 	    }
 	    
-	    mpi = va_arg(ap, BIGNUM *);
-	    BN_init(mpi);
-	    BN_bin2bn(buf, len, mpi);
+	    mpi = va_arg(ap, BIGNUM **);
+            if (mpi) {
+                if (!*mpi) *mpi = BN_new();
+                else BN_clear(*mpi);
+                BN_bin2bn((unsigned char *) buf, len, *mpi);
+            }
 	    break;
 
 	case 'o':
@@ -883,16 +893,17 @@ static int UnBuffer(const sasl_utils_t *
 /*
  * Apply the hash function to the data specifed by the fmt string.
  */
-static int MakeHash(const EVP_MD *md, unsigned char hash[], int *hashlen,
+static int MakeHash(const EVP_MD *md,
+                    unsigned char hash[], unsigned int *hashlen,
 		    const char *fmt, ...)
 {
     va_list ap;
     char *p, buf[4096], *in;
-    int inlen;
-    EVP_MD_CTX mdctx;
+    unsigned int inlen;
+    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
     int r = 0, hflag;
 
-    EVP_DigestInit(&mdctx, md);
+    EVP_DigestInit(mdctx, md);
 
     va_start(ap, fmt);
     for (p = (char *) fmt; *p; p++) {
@@ -910,7 +921,7 @@ static int MakeHash(const EVP_MD *md, un
 		BIGNUM *mval = va_arg(ap, BIGNUM *);
 
 		in = buf;
-		r = BigIntToBytes(mval, buf, sizeof(buf)-1, &inlen);
+		r = BigIntToBytes(mval, (unsigned char *) buf, sizeof(buf)-1, &inlen);
 		if (r) goto done;
 		break;
 	    }
@@ -947,47 +958,52 @@ static int MakeHash(const EVP_MD *md, un
 
 	if (hflag) {
 	    /* hash data separately before adding to current hash */
-	    EVP_MD_CTX tmpctx;
+	    EVP_MD_CTX *tmpctx = EVP_MD_CTX_new();
 
-	    EVP_DigestInit(&tmpctx, md);
-	    EVP_DigestUpdate(&tmpctx, in, inlen);
-	    EVP_DigestFinal(&tmpctx, buf, &inlen);
+	    EVP_DigestInit(tmpctx, md);
+	    EVP_DigestUpdate(tmpctx, in, inlen);
+	    EVP_DigestFinal(tmpctx, (unsigned char *) buf, &inlen);
+            EVP_MD_CTX_free(tmpctx);
 	    in = buf;
 	}
 
-	EVP_DigestUpdate(&mdctx, in, inlen);
+	EVP_DigestUpdate(mdctx, in, inlen);
     }
   done:
     va_end(ap);
 
-    EVP_DigestFinal(&mdctx, hash, hashlen);
+    EVP_DigestFinal(mdctx, hash, hashlen);
+    EVP_MD_CTX_free(mdctx);
 
     return r;
 }
 
 static int CalculateX(context_t *text, const char *salt, int saltlen, 
-		      const char *user, const char *pass, int passlen, 
-		      BIGNUM *x)
+		      const char *user, const unsigned char *pass, int passlen, 
+		      BIGNUM **x)
 {
-    char hash[EVP_MAX_MD_SIZE];
-    int hashlen;
+    unsigned char hash[EVP_MAX_MD_SIZE];
+    unsigned int hashlen;
     
     /* x = H(salt | H(user | ':' | pass)) */
     MakeHash(text->md, hash, &hashlen, "%s:%o", user, passlen, pass);
     MakeHash(text->md, hash, &hashlen, "%o%o", saltlen, salt, hashlen, hash);
     
-    BN_init(x);
-    BN_bin2bn(hash, hashlen, x);
+    *x = BN_new();
+    BN_bin2bn(hash, hashlen, *x);
     
     return SASL_OK;
 }
 
 static int CalculateM1(context_t *text, BIGNUM *N, BIGNUM *g,
 		       char *U, char *salt, int saltlen,
-		       BIGNUM *A, BIGNUM *B, char *K, int Klen,
-		       char *I, char *L, char *M1, int *M1len)
+		       BIGNUM *A, BIGNUM *B,
+                       unsigned char *K, unsigned int Klen,
+		       char *I, char *L,
+                       unsigned char *M1, unsigned int *M1len)
 {
-    int r, i, len;
+    int r;
+    unsigned int i, len;
     unsigned char Nhash[EVP_MAX_MD_SIZE];
     unsigned char ghash[EVP_MAX_MD_SIZE];
     unsigned char Ng[EVP_MAX_MD_SIZE];
@@ -1010,9 +1026,10 @@ static int CalculateM1(context_t *text, 
 }
 
 static int CalculateM2(context_t *text, BIGNUM *A,
-		       char *M1, int M1len, char *K, int Klen,
+		       unsigned char *M1, unsigned int M1len,
+                       unsigned char *K, unsigned int Klen,
 		       char *I, char *o, char *sid, uint32 ttl,
-		       char *M2, int *M2len)
+		       unsigned char *M2, unsigned int *M2len)
 {
     int r;
     
@@ -1386,7 +1403,8 @@ static int SetMDA(srp_options_t *opts, c
  * Setup the selected security layer.
  */
 static int LayerInit(srp_options_t *opts, context_t *text,
-		     sasl_out_params_t *oparams, char *enc_IV, char *dec_IV,
+		     sasl_out_params_t *oparams,
+                     unsigned char *enc_IV, unsigned char *dec_IV,
 		     unsigned maxbufsize)
 {
     layer_option_t *opt;
@@ -1431,8 +1449,10 @@ static int LayerInit(srp_options_t *opts
 
 	/* Initialize the HMACs */
 	text->hmac_md = EVP_get_digestbyname(opt->evp_name);
-	HMAC_Init(&text->hmac_send_ctx, text->K, text->Klen, text->hmac_md);
-	HMAC_Init(&text->hmac_recv_ctx, text->K, text->Klen, text->hmac_md);
+        text->hmac_send_ctx = HMAC_CTX_new();
+	HMAC_Init_ex(text->hmac_send_ctx, text->K, text->Klen, text->hmac_md, NULL);
+        text->hmac_recv_ctx = HMAC_CTX_new();
+	HMAC_Init_ex(text->hmac_recv_ctx, text->K, text->Klen, text->hmac_md, NULL);
 	
 	/* account for HMAC */
 	oparams->maxoutbuf -= EVP_MD_size(text->hmac_md);
@@ -1456,11 +1476,13 @@ static int LayerInit(srp_options_t *opts
 	/* Initialize the ciphers */
 	text->cipher = EVP_get_cipherbyname(opt->evp_name);
 
-	EVP_CIPHER_CTX_init(&text->cipher_enc_ctx);
-	EVP_EncryptInit(&text->cipher_enc_ctx, text->cipher, text->K, enc_IV);
+        text->cipher_enc_ctx = EVP_CIPHER_CTX_new();
+	EVP_CIPHER_CTX_init(text->cipher_enc_ctx);
+	EVP_EncryptInit(text->cipher_enc_ctx, text->cipher, text->K, enc_IV);
 
-	EVP_CIPHER_CTX_init(&text->cipher_dec_ctx);
-	EVP_DecryptInit(&text->cipher_dec_ctx, text->cipher, text->K, dec_IV);
+        text->cipher_dec_ctx = EVP_CIPHER_CTX_new();
+	EVP_CIPHER_CTX_init(text->cipher_dec_ctx);
+	EVP_DecryptInit(text->cipher_dec_ctx, text->cipher, text->K, dec_IV);
     }
     
     return SASL_OK;
@@ -1469,13 +1491,13 @@ static int LayerInit(srp_options_t *opts
 static void LayerCleanup(context_t *text)
 {
     if (text->layer & BIT_INTEGRITY) {
-	HMAC_cleanup(&text->hmac_send_ctx);
-	HMAC_cleanup(&text->hmac_recv_ctx);
+	HMAC_CTX_free(text->hmac_send_ctx);
+	HMAC_CTX_free(text->hmac_recv_ctx);
     }
 
     if (text->layer & BIT_CONFIDENTIALITY) {
-	EVP_CIPHER_CTX_cleanup(&text->cipher_enc_ctx);
-	EVP_CIPHER_CTX_cleanup(&text->cipher_dec_ctx);
+	EVP_CIPHER_CTX_free(text->cipher_enc_ctx);
+	EVP_CIPHER_CTX_free(text->cipher_dec_ctx);
     }
 }
     
@@ -1490,13 +1512,13 @@ static void srp_common_mech_dispose(void
     
     if (!text) return;
     
-    BN_clear_free(&text->N);
-    BN_clear_free(&text->g);
-    BN_clear_free(&text->v);
-    BN_clear_free(&text->b);
-    BN_clear_free(&text->B);
-    BN_clear_free(&text->a);
-    BN_clear_free(&text->A);
+    BN_clear_free(text->N);
+    BN_clear_free(text->g);
+    BN_clear_free(text->v);
+    BN_clear_free(text->b);
+    BN_clear_free(text->B);
+    BN_clear_free(text->a);
+    BN_clear_free(text->A);
     
     if (text->authid)		utils->free(text->authid);
     if (text->userid)		utils->free(text->userid);
@@ -1534,16 +1556,16 @@ srp_common_mech_free(void *global_contex
  *
  * All arithmetic is done modulo N
  */
-static int generate_N_and_g(BIGNUM *N, BIGNUM *g)
+static int generate_N_and_g(BIGNUM **N, BIGNUM **g)
 {
     int result;
-    
-    BN_init(N);
-    result = BN_hex2bn(&N, Ng_tab[NUM_Ng-1].N);
+
+    *N = BN_new();
+    result = BN_hex2bn(N, Ng_tab[NUM_Ng-1].N);
     if (!result) return SASL_FAIL;
     
-    BN_init(g);
-    BN_set_word(g, Ng_tab[NUM_Ng-1].g);
+    *g = BN_new();
+    BN_set_word(*g, Ng_tab[NUM_Ng-1].g);
     
     return SASL_OK;
 }
@@ -1551,10 +1573,10 @@ static int generate_N_and_g(BIGNUM *N, B
 static int CalculateV(context_t *text,
 		      BIGNUM *N, BIGNUM *g,
 		      const char *user,
-		      const char *pass, unsigned passlen,
-		      BIGNUM *v, char **salt, int *saltlen)
+		      const unsigned char *pass, unsigned passlen,
+		      BIGNUM **v, char **salt, int *saltlen)
 {
-    BIGNUM x;
+    BIGNUM *x = NULL;
     BN_CTX *ctx = BN_CTX_new();
     int r;
     
@@ -1572,40 +1594,41 @@ static int CalculateV(context_t *text,
     }
     
     /* v = g^x % N */
-    BN_init(v);
-    BN_mod_exp(v, g, &x, N, ctx);
+    *v = BN_new();
+    BN_mod_exp(*v, g, x, N, ctx);
     
     BN_CTX_free(ctx);
-    BN_clear_free(&x);
+    BN_clear_free(x);
     
     return r;   
 }
 
 static int CalculateB(context_t *text  __attribute__((unused)),
-		      BIGNUM *v, BIGNUM *N, BIGNUM *g, BIGNUM *b, BIGNUM *B)
+		      BIGNUM *v, BIGNUM *N, BIGNUM *g, BIGNUM **b, BIGNUM **B)
 {
-    BIGNUM v3;
+    BIGNUM *v3 = BN_new();
     BN_CTX *ctx = BN_CTX_new();
     
     /* Generate b */
     GetRandBigInt(b);
 	
     /* Per [SRP]: make sure b > log[g](N) -- g is always 2 */
-    BN_add_word(b, BN_num_bits(N));
+    BN_add_word(*b, BN_num_bits(N));
 	
     /* B = (3v + g^b) % N */
-    BN_init(&v3);
-    BN_set_word(&v3, 3);
-    BN_mod_mul(&v3, &v3, v, N, ctx);
-    BN_init(B);
-    BN_mod_exp(B, g, b, N, ctx);
+    BN_set_word(v3, 3);
+    BN_mod_mul(v3, v3, v, N, ctx);
+
+    *B = BN_new();
+    BN_mod_exp(*B, g, *b, N, ctx);
 #if OPENSSL_VERSION_NUMBER >= 0x00907000L
-    BN_mod_add(B, B, &v3, N, ctx);
+    BN_mod_add(*B, *B, v3, N, ctx);
 #else
-    BN_add(B, B, &v3);
-    BN_mod(B, B, N, ctx);
+    BN_add(*B, *B, v3);
+    BN_mod(*B, *B, N, ctx);
 #endif
 
+    BN_clear_free(v3);
     BN_CTX_free(ctx);
     
     return SASL_OK;
@@ -1613,13 +1636,13 @@ static int CalculateB(context_t *text  _
 	
 static int ServerCalculateK(context_t *text, BIGNUM *v,
 			    BIGNUM *N, BIGNUM *A, BIGNUM *b, BIGNUM *B,
-			    char *K, int *Klen)
+			    unsigned char *K, unsigned int *Klen)
 {
     unsigned char hash[EVP_MAX_MD_SIZE];
-    int hashlen;
-    BIGNUM u;
-    BIGNUM base;
-    BIGNUM S;
+    unsigned int hashlen;
+    BIGNUM *u = BN_new();
+    BIGNUM *base = BN_new();
+    BIGNUM *S = BN_new();
     BN_CTX *ctx = BN_CTX_new();
     int r;
     
@@ -1627,50 +1650,47 @@ static int ServerCalculateK(context_t *t
     r = MakeHash(text->md, hash, &hashlen, "%m%m", A, B);
     if (r) return r;
 	
-    BN_init(&u);
-    BN_bin2bn(hash, hashlen, &u);
+    BN_bin2bn(hash, hashlen, u);
 	
     /* S = (Av^u) ^ b % N */
-    BN_init(&base);
-    BN_mod_exp(&base, v, &u, N, ctx);
-    BN_mod_mul(&base, &base, A, N, ctx);
+    BN_mod_exp(base, v, u, N, ctx);
+    BN_mod_mul(base, base, A, N, ctx);
     
-    BN_init(&S);
-    BN_mod_exp(&S, &base, b, N, ctx);
+    BN_mod_exp(S, base, b, N, ctx);
     
     /* per Tom Wu: make sure Av^u != 1 (mod N) */
-    if (BN_is_one(&base)) {
+    if (BN_is_one(base)) {
 	SETERROR(text->utils, "Unsafe SRP value for 'Av^u'\n");
 	r = SASL_BADPROT;
 	goto err;
     }
     
     /* per Tom Wu: make sure Av^u != -1 (mod N) */
-    BN_add_word(&base, 1);
-    if (BN_cmp(&S, N) == 0) {
+    BN_add_word(base, 1);
+    if (BN_cmp(S, N) == 0) {
 	SETERROR(text->utils, "Unsafe SRP value for 'Av^u'\n");
 	r = SASL_BADPROT;
 	goto err;
     }
     
     /* K = H(S) */
-    r = MakeHash(text->md, K, Klen, "%m", &S);
+    r = MakeHash(text->md, K, Klen, "%m", S);
     if (r) goto err;
     
     r = SASL_OK;
     
   err:
     BN_CTX_free(ctx);
-    BN_clear_free(&u);
-    BN_clear_free(&base);
-    BN_clear_free(&S);
+    BN_clear_free(u);
+    BN_clear_free(base);
+    BN_clear_free(S);
     
     return r;
 }
 
 static int ParseUserSecret(const sasl_utils_t *utils,
 			   char *secret, size_t seclen,
-			   char **mda, BIGNUM *v, char **salt, int *saltlen)
+			   char **mda, BIGNUM **v, char **salt, int *saltlen)
 {
     int r;
     
@@ -1678,7 +1698,7 @@ static int ParseUserSecret(const sasl_ut
      *
      *  { utf8(mda) mpi(v) os(salt) }  (base64 encoded)
      */
-    r = utils->decode64(secret, seclen, secret, seclen, &seclen);
+    r = utils->decode64(secret, seclen, secret, seclen, (unsigned *) &seclen);
 
     if (!r)
 	r = UnBuffer(utils, secret, seclen, "%s%m%o", mda, v, saltlen, salt);
@@ -1919,8 +1939,8 @@ static int srp_server_mech_step1(context
 	    goto cleanup;
 	}
 	
-	result = CalculateV(text, &text->N, &text->g, text->authid,
-			    auxprop_values[1].values[0], len,
+	result = CalculateV(text, text->N, text->g, text->authid,
+			    (unsigned char *) auxprop_values[1].values[0], len,
 			    &text->v, &text->salt, &text->saltlen);
 	if (result) {
 	    params->utils->seterror(params->utils->conn, 0, 
@@ -1938,8 +1958,7 @@ static int srp_server_mech_step1(context
     params->utils->prop_erase(params->propctx, password_request[1]);
     
     /* Calculate B */
-    result = CalculateB(text, &text->v, &text->N, &text->g,
-			&text->b, &text->B);
+    result = CalculateB(text, text->v, text->N, text->g, &text->b, &text->B);
     if (result) {
 	params->utils->seterror(params->utils->conn, 0, 
 				"Error calculating B");
@@ -1967,8 +1986,8 @@ static int srp_server_mech_step1(context
      */
     result = MakeBuffer(text->utils, &text->out_buf, &text->out_buf_len,
 			serveroutlen, "%c%m%m%o%m%s",
-			0x00, &text->N, &text->g, text->saltlen, text->salt,
-			&text->B, text->server_options);
+			0x00, text->N, text->g, text->saltlen, text->salt,
+			text->B, text->server_options);
     if (result) {
 	params->utils->seterror(params->utils->conn, 0, 
 				"Error creating SRP buffer from data in step 1");
@@ -1997,15 +2016,15 @@ static int srp_server_mech_step2(context
 			sasl_out_params_t *oparams)
 {
     int result;    
-    char *M1 = NULL, *cIV = NULL; /* don't free */
-    int M1len, cIVlen;
+    unsigned char *M1 = NULL, *cIV = NULL; /* don't free */
+    unsigned int M1len, cIVlen;
     srp_options_t client_opts;
-    char myM1[EVP_MAX_MD_SIZE];
-    int myM1len;
-    int i;
-    char M2[EVP_MAX_MD_SIZE];
-    int M2len;
-    char sIV[SRP_MAXBLOCKSIZE];
+    unsigned char myM1[EVP_MAX_MD_SIZE];
+    unsigned int myM1len;
+    unsigned int i;
+    unsigned char M2[EVP_MAX_MD_SIZE];
+    unsigned int M2len;
+    unsigned char sIV[SRP_MAXBLOCKSIZE];
     
     /* Expect:
      *
@@ -2027,7 +2046,7 @@ static int srp_server_mech_step2(context
     }
     
     /* Per [SRP]: reject A <= 0 */
-    if (BigIntCmpWord(&text->A, 0) <= 0) {
+    if (BigIntCmpWord(text->A, 0) <= 0) {
 	SETERROR(params->utils, "Illegal value for 'A'\n");
 	result = SASL_BADPROT;
 	goto cleanup;
@@ -2058,8 +2077,8 @@ static int srp_server_mech_step2(context
     }
 
     /* Calculate K */
-    result = ServerCalculateK(text, &text->v, &text->N, &text->A,
-			      &text->b, &text->B, text->K, &text->Klen);
+    result = ServerCalculateK(text, text->v, text->N, text->A,
+			      text->b, text->B, text->K, &text->Klen);
     if (result) {
 	params->utils->seterror(params->utils->conn, 0, 
 				"Error calculating K");
@@ -2067,8 +2086,8 @@ static int srp_server_mech_step2(context
     }
     
     /* See if M1 is correct */
-    result = CalculateM1(text, &text->N, &text->g, text->authid,
-			 text->salt, text->saltlen, &text->A, &text->B,
+    result = CalculateM1(text, text->N, text->g, text->authid,
+			 text->salt, text->saltlen, text->A, text->B,
 			 text->K, text->Klen, text->userid,
 			 text->server_options, myM1, &myM1len);
     if (result) {
@@ -2095,7 +2114,7 @@ static int srp_server_mech_step2(context
     }
     
     /* calculate M2 to send */
-    result = CalculateM2(text, &text->A, M1, M1len, text->K, text->Klen,
+    result = CalculateM2(text, text->A, M1, M1len, text->K, text->Klen,
 			 text->userid, text->client_options, "", 0,
 			 M2, &M2len);
     if (result) {
@@ -2105,7 +2124,7 @@ static int srp_server_mech_step2(context
     }
     
     /* Create sIV (server initial vector) */
-    text->utils->rand(text->utils->rpool, sIV, sizeof(sIV));
+    text->utils->rand(text->utils->rpool, (char *) sIV, sizeof(sIV));
     
     /*
      * Send out:
@@ -2230,20 +2249,20 @@ static int srp_setpass(void *glob_contex
     r = _plug_make_fulluser(sparams->utils, &user, user_only, realm);
 
     if (r) {
-	goto end;
+	goto cleanup;
     }
 
     if ((flags & SASL_SET_DISABLE) || pass == NULL) {
 	sec = NULL;
     } else {
-	context_t *text;
-	BIGNUM N;
-	BIGNUM g;
-	BIGNUM v;
+	context_t *text = NULL;
+	BIGNUM *N = NULL;
+	BIGNUM *g = NULL;
+	BIGNUM *v = NULL;
 	char *salt;
 	int saltlen;
 	char *buffer = NULL;
-	int bufferlen, alloclen, encodelen;
+	unsigned int bufferlen, alloclen, encodelen;
 	
 	text = sparams->utils->malloc(sizeof(context_t));
 	if (text == NULL) {
@@ -2264,7 +2283,8 @@ static int srp_setpass(void *glob_contex
 	}
 
 	/* user is a full username here */
-	r = CalculateV(text, &N, &g, user, pass, passlen, &v, &salt, &saltlen);
+	r = CalculateV(text, N, g, user,
+                       (unsigned char *) pass, passlen, &v, &salt, &saltlen);
 	if (r) {
 	    sparams->utils->seterror(sparams->utils->conn, 0, 
 				     "Error calculating v");
@@ -2296,16 +2316,16 @@ static int srp_setpass(void *glob_contex
 	    r = SASL_NOMEM;
 	    goto end;
 	}
-	sparams->utils->encode64(buffer, bufferlen, sec->data, alloclen,
+	sparams->utils->encode64(buffer, bufferlen, (char *) sec->data, alloclen,
 				 &encodelen);
 	sec->len = encodelen;
 	
 	/* Clean everything up */
       end:
 	if (buffer) sparams->utils->free((void *) buffer);
-	BN_clear_free(&N);
-	BN_clear_free(&g);
-	BN_clear_free(&v);
+	BN_clear_free(N);
+	BN_clear_free(g);
+	BN_clear_free(v);
 	sparams->utils->free(text);
 	
 	if (r) return r;
@@ -2319,7 +2339,7 @@ static int srp_setpass(void *glob_contex
 	r = sparams->utils->prop_request(propctx, store_request);
     if (!r)
 	r = sparams->utils->prop_set(propctx, "cmusaslsecretSRP",
-				     (sec ? sec->data : NULL),
+				     (char *) (sec ? sec->data : NULL),
 				     (sec ? sec->len : 0));
     if (!r)
 	r = sparams->utils->auxprop_store(sparams->utils->conn, propctx, user);
@@ -2475,7 +2495,7 @@ static int check_N_and_g(const sasl_util
 }
 
 static int CalculateA(context_t *text  __attribute__((unused)),
-		      BIGNUM *N, BIGNUM *g, BIGNUM *a, BIGNUM *A)
+		      BIGNUM *N, BIGNUM *g, BIGNUM **a, BIGNUM **A)
 {
     BN_CTX *ctx = BN_CTX_new();
     
@@ -2483,11 +2503,11 @@ static int CalculateA(context_t *text  _
     GetRandBigInt(a);
 	
     /* Per [SRP]: make sure a > log[g](N) -- g is always 2 */
-    BN_add_word(a, BN_num_bits(N));
+    BN_add_word(*a, BN_num_bits(N));
 	
     /* A = g^a % N */
-    BN_init(A);
-    BN_mod_exp(A, g, a, N, ctx);
+    *A = BN_new();
+    BN_mod_exp(*A, g, *a, N, ctx);
 
     BN_CTX_free(ctx);
     
@@ -2495,30 +2515,30 @@ static int CalculateA(context_t *text  _
 }
 	
 static int ClientCalculateK(context_t *text, char *salt, int saltlen,
-			    char *user, char *pass, int passlen,
+			    char *user, unsigned char *pass, int passlen,
 			    BIGNUM *N, BIGNUM *g, BIGNUM *a, BIGNUM *A,
-			    BIGNUM *B, char *K, int *Klen)
+			    BIGNUM *B, unsigned char *K, unsigned int *Klen)
 {
     int r;
     unsigned char hash[EVP_MAX_MD_SIZE];
-    int hashlen;
-    BIGNUM x;
-    BIGNUM u;
-    BIGNUM aux;
-    BIGNUM gx;
-    BIGNUM gx3;
-    BIGNUM base;
-    BIGNUM S;
+    unsigned int hashlen;
+    BIGNUM *x = NULL;
+    BIGNUM *u = BN_new();
+    BIGNUM *aux = BN_new();
+    BIGNUM *gx = BN_new();
+    BIGNUM *gx3 = BN_new();
+    BIGNUM *base = BN_new();
+    BIGNUM *S = BN_new();
     BN_CTX *ctx = BN_CTX_new();
     
     /* u = H(A | B) */
     r = MakeHash(text->md, hash, &hashlen, "%m%m", A, B);
     if (r) goto err;
-    BN_init(&u);
-    BN_bin2bn(hash, hashlen, &u);
+    u = BN_new();
+    BN_bin2bn(hash, hashlen, u);
     
     /* per Tom Wu: make sure u != 0 */
-    if (BN_is_zero(&u)) {
+    if (BN_is_zero(u)) {
 	SETERROR(text->utils, "SRP: Illegal value for 'u'\n");
 	r = SASL_BADPROT;
 	goto err;
@@ -2530,48 +2550,43 @@ static int ClientCalculateK(context_t *t
     if (r) return r;
     
     /* a + ux */
-    BN_init(&aux);
-    BN_mul(&aux, &u, &x, ctx);
-    BN_add(&aux, &aux, a);
+    BN_mul(aux, u, x, ctx);
+    BN_add(aux, aux, a);
     
     /* gx3 = 3(g^x) % N */
-    BN_init(&gx);
-    BN_mod_exp(&gx, g, &x, N, ctx);
-    BN_init(&gx3);
-    BN_set_word(&gx3, 3);
-    BN_mod_mul(&gx3, &gx3, &gx, N, ctx);
+    BN_mod_exp(gx, g, x, N, ctx);
+    BN_set_word(gx3, 3);
+    BN_mod_mul(gx3, gx3, gx, N, ctx);
     
     /* base = (B - 3(g^x)) % N */
-    BN_init(&base);
 #if OPENSSL_VERSION_NUMBER >= 0x00907000L
-    BN_mod_sub(&base, B, &gx3, N, ctx);
+    BN_mod_sub(base, B, gx3, N, ctx);
 #else
-    BN_sub(&base, B, &gx3);
-    BN_mod(&base, &base, N, ctx);
-    if (BigIntCmpWord(&base, 0) < 0) {
-	BN_add(&base, &base, N);
+    BN_sub(base, B, gx3);
+    BN_mod(base, base, N, ctx);
+    if (BigIntCmpWord(base, 0) < 0) {
+	BN_add(base, base, N);
     }
 #endif
     
     /* S = base^aux % N */
-    BN_init(&S);
-    BN_mod_exp(&S, &base, &aux, N, ctx);
+    BN_mod_exp(S, base, aux, N, ctx);
     
     /* K = H(S) */
-    r = MakeHash(text->md, K, Klen, "%m", &S);
+    r = MakeHash(text->md, K, Klen, "%m", S);
     if (r) goto err;
     
     r = SASL_OK;
     
   err:
     BN_CTX_free(ctx);
-    BN_clear_free(&x);
-    BN_clear_free(&u);
-    BN_clear_free(&aux);
-    BN_clear_free(&gx);
-    BN_clear_free(&gx3);
-    BN_clear_free(&base);
-    BN_clear_free(&S);
+    BN_clear_free(x);
+    BN_clear_free(u);
+    BN_clear_free(aux);
+    BN_clear_free(gx);
+    BN_clear_free(gx3);
+    BN_clear_free(base);
+    BN_clear_free(S);
     
     return r;
 }
@@ -2709,7 +2724,7 @@ static int srp_client_mech_new(void *glo
     }
     
     memset(text, 0, sizeof(context_t));
-    
+
     text->state = 1;
     text->utils = params->utils;
 
@@ -2866,7 +2881,7 @@ srp_client_mech_step2(context_t *text,
     }
 
     /* Check N and g to see if they are one of the recommended pairs */
-    result = check_N_and_g(params->utils, &text->N, &text->g);
+    result = check_N_and_g(params->utils, text->N, text->g);
     if (result) {
 	params->utils->log(NULL, SASL_LOG_ERR,
 			   "Values of 'N' and 'g' are not recommended\n");
@@ -2874,7 +2889,7 @@ srp_client_mech_step2(context_t *text,
     }
     
     /* Per [SRP]: reject B <= 0, B >= N */
-    if (BigIntCmpWord(&text->B, 0) <= 0 || BN_cmp(&text->B, &text->N) >= 0) {
+    if (BigIntCmpWord(text->B, 0) <= 0 || BN_cmp(text->B, text->N) >= 0) {
 	SETERROR(params->utils, "Illegal value for 'B'\n");
 	result = SASL_BADPROT;
 	goto cleanup;
@@ -2913,7 +2928,7 @@ srp_client_mech_step2(context_t *text,
     }
 
     /* Calculate A */
-    result = CalculateA(text, &text->N, &text->g, &text->a, &text->A);
+    result = CalculateA(text, text->N, text->g, &text->a, &text->A);
     if (result) {
 	params->utils->seterror(params->utils->conn, 0, 
 				"Error calculating A");
@@ -2924,7 +2939,7 @@ srp_client_mech_step2(context_t *text,
     result = ClientCalculateK(text, text->salt, text->saltlen,
 			      (char *) oparams->authid, 
 			      text->password->data, text->password->len,
-			      &text->N, &text->g, &text->a, &text->A, &text->B,
+			      text->N, text->g, text->a, text->A, text->B,
 			      text->K, &text->Klen);
     if (result) {
 	params->utils->log(NULL, SASL_LOG_ERR,
@@ -2933,8 +2948,8 @@ srp_client_mech_step2(context_t *text,
     }
     
     /* Calculate M1 (client evidence) */
-    result = CalculateM1(text, &text->N, &text->g, (char *) oparams->authid,
-			 text->salt, text->saltlen, &text->A, &text->B,
+    result = CalculateM1(text, text->N, text->g, (char *) oparams->authid,
+			 text->salt, text->saltlen, text->A, text->B,
 			 text->K, text->Klen, (char *) oparams->user,
 			 text->server_options, text->M1, &text->M1len);
     if (result) {
@@ -2944,7 +2959,7 @@ srp_client_mech_step2(context_t *text,
     }
 
     /* Create cIV (client initial vector) */
-    text->utils->rand(text->utils->rpool, text->cIV, sizeof(text->cIV));
+    text->utils->rand(text->utils->rpool, (char *) text->cIV, sizeof(text->cIV));
     
     /* Send out:
      *
@@ -2957,7 +2972,7 @@ srp_client_mech_step2(context_t *text,
      */
     result = MakeBuffer(text->utils, &text->out_buf, &text->out_buf_len,
 			clientoutlen, "%m%o%s%o",
-			&text->A, text->M1len, text->M1, text->client_options,
+			text->A, text->M1len, text->M1, text->client_options,
 			sizeof(text->cIV), text->cIV);
     if (result) {
 	params->utils->log(NULL, SASL_LOG_ERR, "Error making output buffer\n");
@@ -2985,13 +3000,13 @@ srp_client_mech_step3(context_t *text,
 		      sasl_out_params_t *oparams)
 {
     int result;    
-    char *M2 = NULL, *sIV = NULL; /* don't free */
+    unsigned char *M2 = NULL, *sIV = NULL; /* don't free */
     char *sid = NULL;
-    int M2len, sIVlen;
+    unsigned int M2len, sIVlen;
     uint32 ttl;
-    int i;
-    char myM2[EVP_MAX_MD_SIZE];
-    int myM2len;
+    unsigned int i;
+    unsigned char myM2[EVP_MAX_MD_SIZE];
+    unsigned int myM2len;
     
     /* Expect:
      *
@@ -3012,7 +3027,7 @@ srp_client_mech_step3(context_t *text,
     }
 
     /* calculate our own M2 */
-    result = CalculateM2(text, &text->A, text->M1, text->M1len,
+    result = CalculateM2(text, text->A, text->M1, text->M1len,
 			 text->K, text->Klen, (char *) oparams->user,
 			 text->client_options, "", 0,
 			 myM2, &myM2len);
