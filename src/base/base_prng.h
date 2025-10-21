// Permuted congruential generator
// Based on https://www.pcg-random.org

typedef struct {
    u64 state;
    u64 increment;
} prng_context;

void prng_seed_r(prng_context* rng, u64 init_state, u64 init_seq);
void prng_seed(u64 init_state, u64 init_seq);

u32 prng_rand_r(prng_context* rng);
u32 prng_rand(void);

// Generates a random number between 0 and 1
f32 prng_randf_r(prng_context* rng);
f32 prng_randf(void);

f32 prng_std_norm_r(prng_context* rng);
f32 prng_std_norm(void);

