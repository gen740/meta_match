# Benchmark Report

## Compile-time

### Compile-time: -fsyntax-only

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 0.733 s | 0.003 s |
| if_match | 0.139 s | 0.003 s |
| glaze | 1.172 s | 0.011 s |

### Compile-time: -c -O1

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 0.904 s | 0.005 s |
| if_match | 0.168 s | 0.002 s |
| glaze | 1.236 s | 0.005 s |

### Compile-time: -c -O3

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 0.988 s | 0.025 s |
| if_match | 0.170 s | 0.002 s |
| glaze | 1.280 s | 0.034 s |

## Runtime

| Dataset | Mode | Approach | CPU time |
| --- | --- | --- | ---: |
| first_byte_distinct_10 | hits_only_randomized | meta_match | 1.57 ns |
| first_byte_distinct_10 | hits_only_randomized | if_match | 2.07 ns |
| first_byte_distinct_10 | hits_only_randomized | glaze | 1.84 ns |
| first_byte_distinct_10 | hit_miss_randomized | meta_match | 1.61 ns |
| first_byte_distinct_10 | hit_miss_randomized | if_match | 2.02 ns |
| first_byte_distinct_10 | hit_miss_randomized | glaze | 1.88 ns |
| first_byte_distinct_10 | miss_only | meta_match | 1.25 ns |
| first_byte_distinct_10 | miss_only | if_match | 1.46 ns |
| first_byte_distinct_10 | miss_only | glaze | 0.70 ns |
| hash_prefix_4byte_255 | hits_only_randomized | meta_match | 5.33 ns |
| hash_prefix_4byte_255 | hits_only_randomized | if_match | 21.58 ns |
| hash_prefix_4byte_255 | hits_only_randomized | glaze | 2.80 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | meta_match | 4.14 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | if_match | 21.77 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | glaze | 1.95 ns |
| hash_prefix_4byte_255 | miss_only | meta_match | 2.41 ns |
| hash_prefix_4byte_255 | miss_only | if_match | 40.44 ns |
| hash_prefix_4byte_255 | miss_only | glaze | 0.88 ns |
| http_headers_50 | hits_only_randomized | meta_match | 3.28 ns |
| http_headers_50 | hits_only_randomized | if_match | 2.52 ns |
| http_headers_50 | hits_only_randomized | glaze | 5.43 ns |
| http_headers_50 | hit_miss_randomized | meta_match | 3.28 ns |
| http_headers_50 | hit_miss_randomized | if_match | 2.69 ns |
| http_headers_50 | hit_miss_randomized | glaze | 5.40 ns |
| http_headers_50 | miss_only | meta_match | 2.43 ns |
| http_headers_50 | miss_only | if_match | 1.35 ns |
| http_headers_50 | miss_only | glaze | 1.47 ns |
| http_methods_10 | hits_only_randomized | meta_match | 2.13 ns |
| http_methods_10 | hits_only_randomized | if_match | 2.03 ns |
| http_methods_10 | hits_only_randomized | glaze | 5.59 ns |
| http_methods_10 | hit_miss_randomized | meta_match | 2.20 ns |
| http_methods_10 | hit_miss_randomized | if_match | 2.08 ns |
| http_methods_10 | hit_miss_randomized | glaze | 5.41 ns |
| http_methods_10 | miss_only | meta_match | 1.32 ns |
| http_methods_10 | miss_only | if_match | 1.28 ns |
| http_methods_10 | miss_only | glaze | 1.74 ns |
| json_fields_10 | hits_only_randomized | meta_match | 1.63 ns |
| json_fields_10 | hits_only_randomized | if_match | 1.92 ns |
| json_fields_10 | hits_only_randomized | glaze | 5.89 ns |
| json_fields_10 | hit_miss_randomized | meta_match | 1.71 ns |
| json_fields_10 | hit_miss_randomized | if_match | 2.03 ns |
| json_fields_10 | hit_miss_randomized | glaze | 5.93 ns |
| json_fields_10 | miss_only | meta_match | 1.34 ns |
| json_fields_10 | miss_only | if_match | 1.33 ns |
| json_fields_10 | miss_only | glaze | 1.11 ns |
| prefix_family_10 | hits_only_randomized | meta_match | 2.35 ns |
| prefix_family_10 | hits_only_randomized | if_match | 2.01 ns |
| prefix_family_10 | hits_only_randomized | glaze | 8.54 ns |
| prefix_family_10 | hit_miss_randomized | meta_match | 2.62 ns |
| prefix_family_10 | hit_miss_randomized | if_match | 1.86 ns |
| prefix_family_10 | hit_miss_randomized | glaze | 8.27 ns |
| prefix_family_10 | miss_only | meta_match | 1.76 ns |
| prefix_family_10 | miss_only | if_match | 0.77 ns |
| prefix_family_10 | miss_only | glaze | 2.12 ns |
| tail_distinct_10 | hits_only_randomized | meta_match | 3.81 ns |
| tail_distinct_10 | hits_only_randomized | if_match | 2.04 ns |
| tail_distinct_10 | hits_only_randomized | glaze | 1.81 ns |
| tail_distinct_10 | hit_miss_randomized | meta_match | 3.74 ns |
| tail_distinct_10 | hit_miss_randomized | if_match | 2.07 ns |
| tail_distinct_10 | hit_miss_randomized | glaze | 1.85 ns |
| tail_distinct_10 | miss_only | meta_match | 2.69 ns |
| tail_distinct_10 | miss_only | if_match | 1.51 ns |
| tail_distinct_10 | miss_only | glaze | 2.25 ns |
