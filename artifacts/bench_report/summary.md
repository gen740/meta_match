# Benchmark Report

## Compile-time

### Compile-time: -fsyntax-only

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 1.101 s | 0.009 s |
| if_match | 0.208 s | 0.002 s |
| glaze | 1.802 s | 0.011 s |

### Compile-time: -c -O1

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 1.372 s | 0.025 s |
| if_match | 0.251 s | 0.008 s |
| glaze | 1.962 s | 0.030 s |

### Compile-time: -c -O3

| Approach | Mean | Stddev |
| --- | ---: | ---: |
| meta_match | 1.434 s | 0.015 s |
| if_match | 0.256 s | 0.002 s |
| glaze | 1.979 s | 0.156 s |

## Runtime

| Dataset | Mode | Approach | CPU time |
| --- | --- | --- | ---: |
| first_byte_distinct_10 | hits_only_randomized | meta_match | 2.20 ns |
| first_byte_distinct_10 | hits_only_randomized | if_match | 2.74 ns |
| first_byte_distinct_10 | hits_only_randomized | glaze | 2.53 ns |
| first_byte_distinct_10 | hit_miss_randomized | meta_match | 2.25 ns |
| first_byte_distinct_10 | hit_miss_randomized | if_match | 2.76 ns |
| first_byte_distinct_10 | hit_miss_randomized | glaze | 2.60 ns |
| first_byte_distinct_10 | miss_only | meta_match | 1.78 ns |
| first_byte_distinct_10 | miss_only | if_match | 2.02 ns |
| first_byte_distinct_10 | miss_only | glaze | 0.96 ns |
| hash_prefix_4byte_255 | hits_only_randomized | meta_match | 5.65 ns |
| hash_prefix_4byte_255 | hits_only_randomized | if_match | 29.52 ns |
| hash_prefix_4byte_255 | hits_only_randomized | glaze | 3.26 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | meta_match | 5.51 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | if_match | 29.97 ns |
| hash_prefix_4byte_255 | hit_miss_randomized | glaze | 3.08 ns |
| hash_prefix_4byte_255 | miss_only | meta_match | 3.36 ns |
| hash_prefix_4byte_255 | miss_only | if_match | 55.03 ns |
| hash_prefix_4byte_255 | miss_only | glaze | 0.98 ns |
| http_headers_50 | hits_only_randomized | meta_match | 4.40 ns |
| http_headers_50 | hits_only_randomized | if_match | 5.06 ns |
| http_headers_50 | hits_only_randomized | glaze | 7.06 ns |
| http_headers_50 | hit_miss_randomized | meta_match | 4.54 ns |
| http_headers_50 | hit_miss_randomized | if_match | 5.23 ns |
| http_headers_50 | hit_miss_randomized | glaze | 7.13 ns |
| http_headers_50 | miss_only | meta_match | 3.49 ns |
| http_headers_50 | miss_only | if_match | 3.67 ns |
| http_headers_50 | miss_only | glaze | 2.02 ns |
| http_methods_10 | hits_only_randomized | meta_match | 2.98 ns |
| http_methods_10 | hits_only_randomized | if_match | 2.64 ns |
| http_methods_10 | hits_only_randomized | glaze | 7.70 ns |
| http_methods_10 | hit_miss_randomized | meta_match | 2.98 ns |
| http_methods_10 | hit_miss_randomized | if_match | 2.80 ns |
| http_methods_10 | hit_miss_randomized | glaze | 7.54 ns |
| http_methods_10 | miss_only | meta_match | 1.82 ns |
| http_methods_10 | miss_only | if_match | 1.71 ns |
| http_methods_10 | miss_only | glaze | 2.50 ns |
| json_fields_10 | hits_only_randomized | meta_match | 2.22 ns |
| json_fields_10 | hits_only_randomized | if_match | 3.86 ns |
| json_fields_10 | hits_only_randomized | glaze | 7.79 ns |
| json_fields_10 | hit_miss_randomized | meta_match | 2.34 ns |
| json_fields_10 | hit_miss_randomized | if_match | 3.84 ns |
| json_fields_10 | hit_miss_randomized | glaze | 7.61 ns |
| json_fields_10 | miss_only | meta_match | 1.81 ns |
| json_fields_10 | miss_only | if_match | 1.83 ns |
| json_fields_10 | miss_only | glaze | 1.54 ns |
| prefix_family_10 | hits_only_randomized | meta_match | 3.30 ns |
| prefix_family_10 | hits_only_randomized | if_match | 2.74 ns |
| prefix_family_10 | hits_only_randomized | glaze | 10.89 ns |
| prefix_family_10 | hit_miss_randomized | meta_match | 3.38 ns |
| prefix_family_10 | hit_miss_randomized | if_match | 2.72 ns |
| prefix_family_10 | hit_miss_randomized | glaze | 10.47 ns |
| prefix_family_10 | miss_only | meta_match | 2.36 ns |
| prefix_family_10 | miss_only | if_match | 1.06 ns |
| prefix_family_10 | miss_only | glaze | 2.91 ns |
| tail_distinct_10 | hits_only_randomized | meta_match | 5.22 ns |
| tail_distinct_10 | hits_only_randomized | if_match | 3.07 ns |
| tail_distinct_10 | hits_only_randomized | glaze | 2.56 ns |
| tail_distinct_10 | hit_miss_randomized | meta_match | 5.12 ns |
| tail_distinct_10 | hit_miss_randomized | if_match | 3.19 ns |
| tail_distinct_10 | hit_miss_randomized | glaze | 2.62 ns |
| tail_distinct_10 | miss_only | meta_match | 3.70 ns |
| tail_distinct_10 | miss_only | if_match | 2.17 ns |
| tail_distinct_10 | miss_only | glaze | 1.28 ns |
