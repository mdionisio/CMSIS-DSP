/******************************************************************************
 * @file     filtering_functions_ct_impl.h
 * @brief    Implementation of the FIR filters specialized at compile time
 * @version  V1.0.0
 * @date     14 November 2026
 * Target Processor: Cortex-M and Cortex-A cores
 ******************************************************************************/
/*
 * Copyright (c) 2026 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file has two parts :
 *
 * - The public part (ARM_CT_TEMPLATE_T not defined) defines the
 *   ARM_FIR_*_CT_DEFINE macros and includes the template part once for each
 *   supported data type.
 *
 * - The template part (ARM_CT_TEMPLATE_T defined) contains the generic
 *   kernels written once for all data types. Inside the kernels, the number
 *   of taps and the factors are function arguments but the kernels are
 *   always inlined in functions where those arguments are constants. So the
 *   compiler can fully unroll the loops depending on the number of taps,
 *   remove the tail loops and use constant offsets and strides.
 *
 * The data type dependent operations are described by traits :
 * - ARM_CT_ACC_TYPE_<T>  : accumulator type
 * - ARM_CT_OUT_<T>(acc)  : conversion of the accumulator to the output type
 *
 * When a SIMD unit is available (Neon or Helium) and the type is a floating
 * point type, a vector kernel is used. It is written with the generic
 * vector extension of the compiler (vector_size attribute) so the same code
 * is used for Neon and Helium and for all the floating point types.
 * The vector kernels can be disabled with ARM_CT_DISABLE_VECTOR or forced
 * with ARM_CT_ENABLE_VECTOR (for any target supporting the vector_size
 * attribute).
 */

#if !defined(ARM_CT_TEMPLATE_T)

#ifndef FILTERING_FUNCTIONS_CT_IMPL_H_
#define FILTERING_FUNCTIONS_CT_IMPL_H_

#include "dsp/filtering_functions_ct.h"

/*
 * Optional run-time checks of the consistency between the instance and the
 * compile-time parameters. Disabled by default. When ARM_MATH_CT_CHECK is
 * defined, ARM_CT_ASSERT is used (assert() of the C library by default, so
 * the checks are also removed when NDEBUG is defined).
 */
#if defined(ARM_MATH_CT_CHECK)
  #if !defined(ARM_CT_ASSERT)
    #include <assert.h>
    #define ARM_CT_ASSERT(COND)     assert(COND)
  #endif
  #define ARM_CT_CHECK(COND)        ARM_CT_ASSERT(COND)
#else
  #define ARM_CT_CHECK(COND)        ((void) 0)
#endif

/*
 * restrict qualifiers of the pointers of the kernels. They are only used by
 * the kernels (inlined in the specialized functions) : the prototypes of the
 * functions do not change. __restrict is supported in C and C++ by the
 * compilers supported by CMSIS-DSP.
 *
 * The state buffer is always restrict qualified : it must not overlap the
 * coefficients, the input and the output buffers. The coefficients are
 * restrict qualified when ARM_MATH_CT_RESTRICT_COEFFS is defined. The input
 * and the output buffers are never restrict qualified : they may be the same
 * buffer (in-place processing).
 */
#define ARM_CT_RESTRICT_STATE       __restrict
#if defined(ARM_MATH_CT_RESTRICT_COEFFS)
  #define ARM_CT_RESTRICT_COEFFS    __restrict
#else
  #define ARM_CT_RESTRICT_COEFFS
#endif

#ifdef   __cplusplus
extern "C"
{
#endif

/** @cond ARM_CT_INTERNAL */

/* Full unrolling of loops whose trip count is a compile-time constant */
#if defined(__clang__)
  #define ARM_CT_UNROLL         _Pragma("clang loop unroll(full)")
  #define ARM_CT_UNROLL_BY_4    _Pragma("clang loop unroll_count(4)")
#elif defined(__GNUC__)
  #define ARM_CT_UNROLL         _Pragma("GCC unroll 64")
  #define ARM_CT_UNROLL_BY_4    _Pragma("GCC unroll 4")
#else
  #define ARM_CT_UNROLL
  #define ARM_CT_UNROLL_BY_4
#endif


/* Accumulator types */
#define ARM_CT_ACC_TYPE_f64         float64_t
#define ARM_CT_ACC_TYPE_f32         float32_t
#define ARM_CT_ACC_TYPE_f16         float16_t
#define ARM_CT_ACC_TYPE_q31         q63_t
#define ARM_CT_ACC_TYPE_q15         q63_t
#define ARM_CT_ACC_TYPE_q7          q31_t

/* Conversion of the accumulator to the output type
   (same rounding and saturation as the generic functions) */
#define ARM_CT_OUT_f64(ACC)         (ACC)
#define ARM_CT_OUT_f32(ACC)         (ACC)
#define ARM_CT_OUT_f16(ACC)         (ACC)
#define ARM_CT_OUT_q31(ACC)         ((q31_t) ((ACC) >> 31))
#define ARM_CT_OUT_q15(ACC)         ((q15_t) __SSAT((q31_t) ((ACC) >> 15), 16))
#define ARM_CT_OUT_q7(ACC)          ((q7_t) __SSAT((ACC) >> 7, 8))

/* Number of outputs computed in parallel by the scalar kernels.
   The 64-bit accumulators of q31 and q15 use two registers on 32-bit
   cores so fewer accumulators are used to avoid register spilling. */
#if !defined(ARM_CT_NB_ACC_f64)
  #define ARM_CT_NB_ACC_f64         4U
#endif
#if !defined(ARM_CT_NB_ACC_f32)
  #define ARM_CT_NB_ACC_f32         8U
#endif
#if !defined(ARM_CT_NB_ACC_f16)
  #define ARM_CT_NB_ACC_f16         8U
#endif
#if !defined(ARM_CT_NB_ACC_q31)
  #if defined(__aarch64__)
    #define ARM_CT_NB_ACC_q31       4U
  #else
    #define ARM_CT_NB_ACC_q31       3U
  #endif
#endif
#if !defined(ARM_CT_NB_ACC_q15)
  #if defined(__aarch64__)
    #define ARM_CT_NB_ACC_q15       4U
  #else
    #define ARM_CT_NB_ACC_q15       2U
  #endif
#endif
#if !defined(ARM_CT_NB_ACC_q7)
  #define ARM_CT_NB_ACC_q7          4U
#endif

/* Vector kernels */
#if !defined(ARM_CT_VECTOR_BYTES)
  #define ARM_CT_VECTOR_BYTES       16
#endif

/* Number of vector accumulators of the vector kernels */
#if !defined(ARM_CT_NB_VECTOR_ACC)
  #if defined(ARM_MATH_NEON)
    #define ARM_CT_NB_VECTOR_ACC    4U
  #else
    #define ARM_CT_NB_VECTOR_ACC    2U
  #endif
#endif

/* Above this number of taps, the loops on the taps of the vector kernels
   are not fully unrolled (code size) */
#if !defined(ARM_CT_FULL_UNROLL_MAX_TAPS)
  #define ARM_CT_FULL_UNROLL_MAX_TAPS 128U
#endif

/* Size in bytes of the floating point types (selection of the permutations) */
#define ARM_CT_DATA_BYTES_f64       8
#define ARM_CT_DATA_BYTES_f32       4
#define ARM_CT_DATA_BYTES_f16       2

/* Algorithm of the vector FIR :
   - 1 : the delayed samples are built from aligned windows with
         permutations (vext). Best when the unaligned vector loads are
         expensive (Neon).
   - 0 : the delayed samples are read with unaligned vector loads.
         Best when there is no vext instruction (Helium). */
#if !defined(ARM_CT_VECTOR_EXT)
  #if defined(ARM_MATH_NEON) && (defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 12)))
    #define ARM_CT_VECTOR_EXT       1
  #else
    #define ARM_CT_VECTOR_EXT       0
  #endif
#endif

/* The vector kernels use the vector extension of GCC and Clang (armclang).
   With other compilers, the scalar kernels are used. */
#if defined(__GNUC__) || defined(__clang__)
  #define ARM_CT_COMPILER_VECTOR    1
#else
  #define ARM_CT_COMPILER_VECTOR    0
#endif

#if defined(ARM_CT_ENABLE_VECTOR) && !ARM_CT_COMPILER_VECTOR
  #error "ARM_CT_ENABLE_VECTOR : the vector kernels require GCC or Clang"
#endif

#if !defined(ARM_CT_DISABLE_VECTOR) && ARM_CT_COMPILER_VECTOR && (defined(ARM_CT_ENABLE_VECTOR) || defined(ARM_MATH_NEON) || defined(ARM_MATH_MVEF))
  #define ARM_CT_VECTOR_f32         1
#else
  #define ARM_CT_VECTOR_f32         0
#endif

#if !defined(ARM_CT_DISABLE_VECTOR) && ARM_CT_COMPILER_VECTOR && (defined(ARM_CT_ENABLE_VECTOR) || (defined(ARM_MATH_NEON) && defined(__aarch64__)))
  #define ARM_CT_VECTOR_f64         1
#else
  #define ARM_CT_VECTOR_f64         0
#endif

#if !defined(ARM_CT_DISABLE_VECTOR) && ARM_CT_COMPILER_VECTOR && (defined(ARM_CT_ENABLE_VECTOR) || defined(ARM_MATH_NEON_FLOAT16) || defined(ARM_MATH_MVE_FLOAT16))
  #define ARM_CT_VECTOR_f16         1
#else
  #define ARM_CT_VECTOR_f16         0
#endif

/* Instantiation of the kernels for each data type */
#define ARM_CT_TEMPLATE_T           f64
#define ARM_CT_TEMPLATE_VECTOR      ARM_CT_VECTOR_f64
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR

#define ARM_CT_TEMPLATE_T           f32
#define ARM_CT_TEMPLATE_VECTOR      ARM_CT_VECTOR_f32
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR

#if defined(ARM_FLOAT16_SUPPORTED)
#define ARM_CT_TEMPLATE_T           f16
#define ARM_CT_TEMPLATE_VECTOR      ARM_CT_VECTOR_f16
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR
#endif

#define ARM_CT_TEMPLATE_T           q31
#define ARM_CT_TEMPLATE_VECTOR      0
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR

#define ARM_CT_TEMPLATE_T           q15
#define ARM_CT_TEMPLATE_VECTOR      0
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR

#define ARM_CT_TEMPLATE_T           q7
#define ARM_CT_TEMPLATE_VECTOR      0
#include "dsp/filtering_functions_ct_impl.h"
#undef  ARM_CT_TEMPLATE_T
#undef  ARM_CT_TEMPLATE_VECTOR

/** @endcond */

/**
 * @addtogroup FIR_CT
 * @{
 */

/**
 * @brief Define a FIR filter specialized for a number of taps
 * @param[in] T        type suffix (f64, f32, f16, q31, q15, q7)
 * @param[in] NUMTAPS  number of taps
 *
 * Defines the function ARM_FIR_CT_FUNC(T, NUMTAPS).
 * The result is the same as arm_fir_<T>() (for the floating point types,
 * the order of the accumulations may be different).
 */
#define ARM_FIR_CT_DEFINE(T, NUMTAPS)                                        \
  ARM_CT_STATIC_ASSERT((NUMTAPS) >= 1, "numTaps must be >= 1");              \
  ARM_CT_FUNC_ATTRIBUTE ARM_FIR_CT_PROTO(T, NUMTAPS)                         \
  {                                                                          \
    ARM_CT_CHECK(S->numTaps == (NUMTAPS));                                   \
    ARM_CT_CAT2(arm_fir_ct_kernel_, T)(S->pState, S->pCoeffs,                \
                                       pSrc, pDst, blockSize,                \
                                       (uint32_t) (NUMTAPS));                \
  }

/**
 * @brief Define a FIR decimator specialized for a number of taps and a decimation factor
 * @param[in] T        type suffix (f64, f32, q31, q15)
 * @param[in] NUMTAPS  number of taps
 * @param[in] DECIM_M  decimation factor M
 *
 * Defines the function ARM_FIR_DECIMATE_CT_FUNC(T, NUMTAPS, DECIM_M).
 * blockSize must be a multiple of M.
 */
#define ARM_FIR_DECIMATE_CT_DEFINE(T, NUMTAPS, DECIM_M)                      \
  ARM_CT_STATIC_ASSERT((NUMTAPS) >= 1, "numTaps must be >= 1");              \
  ARM_CT_STATIC_ASSERT(((DECIM_M) >= 1) && ((DECIM_M) <= 255),               \
                       "M must be in [1,255]");                              \
  ARM_CT_FUNC_ATTRIBUTE ARM_FIR_DECIMATE_CT_PROTO(T, NUMTAPS, DECIM_M)       \
  {                                                                          \
    ARM_CT_CHECK(S->numTaps == (NUMTAPS));                                   \
    ARM_CT_CHECK(S->M == (DECIM_M));                                         \
    ARM_CT_CHECK((blockSize % (DECIM_M)) == 0U);                             \
    ARM_CT_CAT2(arm_fir_decimate_ct_kernel_, T)(S->pState, S->pCoeffs,       \
                                                pSrc, pDst, blockSize,       \
                                                (uint32_t) (NUMTAPS),        \
                                                (uint32_t) (DECIM_M));       \
  }

/**
 * @brief Define a FIR interpolator specialized for a number of taps and an interpolation factor
 * @param[in] T        type suffix (f32, q31, q15)
 * @param[in] NUMTAPS  number of taps (multiple of L)
 * @param[in] INTERP_L interpolation factor L
 *
 * Defines the function ARM_FIR_INTERPOLATE_CT_FUNC(T, NUMTAPS, INTERP_L).
 */
#define ARM_FIR_INTERPOLATE_CT_DEFINE(T, NUMTAPS, INTERP_L)                  \
  ARM_CT_STATIC_ASSERT(((INTERP_L) >= 1) && ((INTERP_L) <= 255),             \
                       "L must be in [1,255]");                              \
  ARM_CT_STATIC_ASSERT(((NUMTAPS) >= (INTERP_L)) &&                          \
                       (((NUMTAPS) % (INTERP_L)) == 0),                      \
                       "numTaps must be a non zero multiple of L");          \
  ARM_CT_FUNC_ATTRIBUTE ARM_FIR_INTERPOLATE_CT_PROTO(T, NUMTAPS, INTERP_L)   \
  {                                                                          \
    ARM_CT_CHECK(S->L == (INTERP_L));                                        \
    ARM_CT_CHECK(S->phaseLength == ((NUMTAPS) / (INTERP_L)));                \
    ARM_CT_CAT2(arm_fir_interpolate_ct_kernel_, T)(S->pState, S->pCoeffs,    \
                                                   pSrc, pDst, blockSize,    \
                                                   (uint32_t) ((NUMTAPS) / (INTERP_L)), \
                                                   (uint32_t) (INTERP_L));   \
  }

/**
 * @} end of FIR_CT group
 */

#ifdef   __cplusplus
}
#endif

#endif /* ifndef FILTERING_FUNCTIONS_CT_IMPL_H_ */

#else /* !defined(ARM_CT_TEMPLATE_T) */

/* ----------------------------------------------------------------------
 * Template part : generic kernels for the type ARM_CT_TEMPLATE_T
 * -------------------------------------------------------------------- */

#define ARM_CT_DT                   ARM_CT_DATA_TYPE(ARM_CT_TEMPLATE_T)
#define ARM_CT_AT                   ARM_CT_CAT2(ARM_CT_ACC_TYPE_, ARM_CT_TEMPLATE_T)
#define ARM_CT_OUT(ACC)             ARM_CT_CAT2(ARM_CT_OUT_, ARM_CT_TEMPLATE_T)(ACC)
#define ARM_CT_NB_ACC               ARM_CT_CAT2(ARM_CT_NB_ACC_, ARM_CT_TEMPLATE_T)
#define ARM_CT_FN(NAME)             ARM_CT_CAT2(NAME, ARM_CT_TEMPLATE_T)

/**
  @brief         Dot product with a strided coefficient buffer.
  @param[in]     px       points to the samples
  @param[in]     pb       points to the coefficients
  @param[in]     nb       number of products
  @param[in]     stride   coefficient stride
  @return        accumulator
 */
__STATIC_FORCEINLINE ARM_CT_AT ARM_CT_FN(arm_ct_dot_)(
  const ARM_CT_DT * px,
  const ARM_CT_DT * pb,
        uint32_t nb,
        uint32_t stride)
{
  ARM_CT_AT acc = 0;

  for (uint32_t i = 0U; i < nb; i++)
  {
    acc += (ARM_CT_AT) px[i] * pb[i * stride];
  }

  return (acc);
}

/**
  @brief         Generic scalar block of the FIR and FIR interpolator.
                 Computes ARM_CT_NB_ACC consecutive outputs.
  @param[in]     px       points to the first state sample
  @param[in]     pb       points to the first coefficient
  @param[out]    pDst     points to the first output
  @param[in]     numTaps  number of taps
  @param[in]     stride   coefficient stride (1 for FIR, L for interpolator)
  @param[in]     dstStride  output stride (1 for FIR, L for interpolator)

  The samples are kept in a sliding window of ARM_CT_NB_ACC registers
  addressed modulo ARM_CT_NB_ACC : since numTaps is a constant, all the
  indices are constants once the loops are unrolled and no register moves
  are needed.
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_ct_fir_block_)(
  const ARM_CT_DT * px,
  const ARM_CT_DT * pb,
        ARM_CT_DT * pDst,
        uint32_t numTaps,
        uint32_t stride,
        uint32_t dstStride)
{
  const uint32_t K = ARM_CT_NB_ACC;
  ARM_CT_AT acc[ARM_CT_NB_ACC];
  ARM_CT_DT x[ARM_CT_NB_ACC];
  uint32_t t, p, k;

  ARM_CT_UNROLL
  for (k = 0U; k < K; k++)
  {
    acc[k] = 0;
  }

  ARM_CT_UNROLL
  for (k = 0U; k < K - 1U; k++)
  {
    x[k] = px[k];
  }

  /* Loop on the taps by groups of K : the window index is (p + k) % K */
  for (t = 0U; t + K <= numTaps; t += K)
  {
    ARM_CT_UNROLL
    for (p = 0U; p < K; p++)
    {
      const ARM_CT_DT c = pb[(t + p) * stride];

      x[(p + K - 1U) % K] = px[t + p + K - 1U];

      ARM_CT_UNROLL
      for (k = 0U; k < K; k++)
      {
        acc[k] += (ARM_CT_AT) x[(p + k) % K] * c;
      }
    }
  }

  /* Remaining taps : numTaps % K is a constant */
  ARM_CT_UNROLL
  for (p = 0U; p < numTaps % K; p++)
  {
    const ARM_CT_DT c = pb[(t + p) * stride];

    x[(p + K - 1U) % K] = px[t + p + K - 1U];

    ARM_CT_UNROLL
    for (k = 0U; k < K; k++)
    {
      acc[k] += (ARM_CT_AT) x[(p + k) % K] * c;
    }
  }

  ARM_CT_UNROLL
  for (k = 0U; k < K; k++)
  {
    pDst[k * dstStride] = ARM_CT_OUT(acc[k]);
  }
}

#if ARM_CT_TEMPLATE_VECTOR

/* Generic vector of ARM_CT_VECTOR_BYTES bytes, used for the values (variables,
   arguments, results). */
typedef ARM_CT_DT ARM_CT_FN(arm_ct_vec_)
  __attribute__((vector_size(ARM_CT_VECTOR_BYTES)));

/* Same vector used only to access the memory : the alignment is the one of
   the scalar type so that unaligned accesses are allowed, and the type may
   alias the scalar type (like the unaligned types of the intrinsics headers).
   It is not used for arguments : GCC 14 for AArch64 fails (internal compiler
   error in aarch64_function_arg_alignment) when an under-aligned vector is
   passed by value. */
typedef ARM_CT_DT ARM_CT_FN(arm_ct_vec_mem_)
  __attribute__((vector_size(ARM_CT_VECTOR_BYTES), aligned(sizeof(ARM_CT_DT)), __may_alias__));

#define ARM_CT_VEC                  ARM_CT_FN(arm_ct_vec_)
#define ARM_CT_VEC_MEM              ARM_CT_FN(arm_ct_vec_mem_)
#define ARM_CT_VL                   ((uint32_t) (ARM_CT_VECTOR_BYTES / sizeof(ARM_CT_DT)))
#define ARM_CT_VK                   ARM_CT_NB_VECTOR_ACC
#define ARM_CT_VLD(P)               ((ARM_CT_VEC) *(const ARM_CT_VEC_MEM *) (P))
#define ARM_CT_VST(P, V)            (*(ARM_CT_VEC_MEM *) (P) = (ARM_CT_VEC_MEM) (V))
#define ARM_CT_VLEN                 (ARM_CT_VECTOR_BYTES / ARM_CT_CAT2(ARM_CT_DATA_BYTES_, ARM_CT_TEMPLATE_T))

/**
  @brief         Vector with all the elements set to zero.
  @return        zero vector
 */
__STATIC_FORCEINLINE ARM_CT_VEC ARM_CT_FN(arm_ct_vzero_)(void)
{
  ARM_CT_VEC v = { 0 };

  return (v);
}

/**
  @brief         Vector with all the elements set to a scalar.
  @param[in]     c        scalar
  @return        { c, c, ..., c }

  gcc does not recognize the elements written one by one as a broadcast
  (one insertion per element) : with gcc, the vector is initialized with all
  the elements in the initializer and a single dup / splat instruction is
  generated. A vector - scalar operation is not used because float16_t is
  promoted to float in C++.
  clang recognizes both forms but its unrolling heuristics change with the
  initializer (slower FIR interpolator) : the elements are written one by
  one with clang.
 */
__STATIC_FORCEINLINE ARM_CT_VEC ARM_CT_FN(arm_ct_vdup_)(ARM_CT_DT c)
{
#if defined(__clang__)
  ARM_CT_VEC v = { 0 };
  uint32_t i;

  ARM_CT_UNROLL
  for (i = 0U; i < ARM_CT_VL; i++)
  {
    v[i] = c;
  }
#elif ARM_CT_VLEN == 2
  ARM_CT_VEC v = { c, c };
#elif ARM_CT_VLEN == 4
  ARM_CT_VEC v = { c, c, c, c };
#elif ARM_CT_VLEN == 8
  ARM_CT_VEC v = { c, c, c, c, c, c, c, c };
#elif ARM_CT_VLEN == 16
  ARM_CT_VEC v = { c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c };
#else
  #error "ARM_CT_VECTOR_BYTES : unsupported vector length"
#endif

  return (v);
}

#if ARM_CT_VECTOR_EXT
/**
  @brief         Extract a vector from the concatenation of two vectors.
  @param[in]     a        first vector
  @param[in]     b        second vector
  @param[in]     n        index of the first element (constant in [0, VL-1])
  @return        { a[n], ..., a[VL-1], b[0], ..., b[n-1] }
 */
__STATIC_FORCEINLINE ARM_CT_VEC ARM_CT_FN(arm_ct_vext_)(
  ARM_CT_VEC a,
  ARM_CT_VEC b,
  uint32_t n)
{
  switch (n)
  {
#if ARM_CT_VLEN == 2
    case 1U: return __builtin_shufflevector(a, b, 1, 2);
#elif ARM_CT_VLEN == 4
    case 1U: return __builtin_shufflevector(a, b, 1, 2, 3, 4);
    case 2U: return __builtin_shufflevector(a, b, 2, 3, 4, 5);
    case 3U: return __builtin_shufflevector(a, b, 3, 4, 5, 6);
#elif ARM_CT_VLEN == 8
    case 1U: return __builtin_shufflevector(a, b, 1, 2, 3, 4, 5, 6, 7, 8);
    case 2U: return __builtin_shufflevector(a, b, 2, 3, 4, 5, 6, 7, 8, 9);
    case 3U: return __builtin_shufflevector(a, b, 3, 4, 5, 6, 7, 8, 9, 10);
    case 4U: return __builtin_shufflevector(a, b, 4, 5, 6, 7, 8, 9, 10, 11);
    case 5U: return __builtin_shufflevector(a, b, 5, 6, 7, 8, 9, 10, 11, 12);
    case 6U: return __builtin_shufflevector(a, b, 6, 7, 8, 9, 10, 11, 12, 13);
    case 7U: return __builtin_shufflevector(a, b, 7, 8, 9, 10, 11, 12, 13, 14);
#else
  #error "ARM_CT_VECTOR_EXT : unsupported vector length"
#endif
    default: return a;
  }
}
#endif /* ARM_CT_VECTOR_EXT */

#if ARM_CT_VECTOR_EXT
/**
  @brief         Accumulation of a group of VL taps.
  @param[in,out] acc      nbAcc vector accumulators
  @param[in]     px       points to the first sample of the group
  @param[in]     pb       points to the first coefficient of the group
  @param[in]     stride   coefficient stride
  @param[in]     nbAcc    number of vector accumulators (constant)
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_ct_fir_vec_group_)(
        ARM_CT_VEC * acc,
  const ARM_CT_DT * px,
  const ARM_CT_DT * pb,
        uint32_t stride,
        uint32_t nbAcc)
{
  ARM_CT_VEC w[ARM_CT_NB_VECTOR_ACC + 1U];
  ARM_CT_DT c[ARM_CT_VLEN];
  uint32_t p, k;

  ARM_CT_UNROLL
  for (k = 0U; k <= nbAcc; k++)
  {
    w[k] = ARM_CT_VLD(px + k * ARM_CT_VL);
  }

  if (stride == 1U)
  {
    const ARM_CT_VEC cv = ARM_CT_VLD(pb);

    ARM_CT_UNROLL
    for (p = 0U; p < ARM_CT_VL; p++)
    {
      c[p] = cv[p];
    }
  }
  else
  {
    ARM_CT_UNROLL
    for (p = 0U; p < ARM_CT_VL; p++)
    {
      c[p] = pb[p * stride];
    }
  }

  ARM_CT_UNROLL
  for (p = 0U; p < ARM_CT_VL; p++)
  {
    ARM_CT_UNROLL
    for (k = 0U; k < nbAcc; k++)
    {
      acc[k] += ARM_CT_FN(arm_ct_vext_)(w[k], w[k + 1U], p) * ARM_CT_FN(arm_ct_vdup_)(c[p]);
    }
  }
}
#endif /* ARM_CT_VECTOR_EXT */

/**
  @brief         Vector FIR / FIR interpolator block.
                 Computes nbAcc * VL consecutive outputs.
  @param[in]     px       points to the first state sample
  @param[in]     pb       points to the first coefficient
  @param[out]    pDst     points to the first output
  @param[in]     numTaps  number of taps
  @param[in]     stride   coefficient stride (1 for FIR, L for interpolator)
  @param[in]     dstStride  output stride (1 for FIR, L for interpolator)
  @param[in]     nbAcc    number of vector accumulators (constant, <= ARM_CT_NB_VECTOR_ACC)

  When ARM_CT_VECTOR_EXT is set, the taps are processed by groups of VL :
  nbAcc + 1 consecutive windows are loaded for each group and the delayed
  samples are extracted with permutations. Once the loop on the taps is
  unrolled, the compiler reuses the windows loaded by the previous group so
  only one window is loaded per group. The last group is always processed
  with unaligned loads so that no sample is read after the end of the state
  buffer.
  Otherwise, for each tap, the samples are read with unaligned vector loads.
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_ct_fir_vec_block_)(
  const ARM_CT_DT * px,
  const ARM_CT_DT * pb,
        ARM_CT_DT * pDst,
        uint32_t numTaps,
        uint32_t stride,
        uint32_t dstStride,
        uint32_t nbAcc)
{
  ARM_CT_VEC acc[ARM_CT_NB_VECTOR_ACC];
  uint32_t t = 0U, k, v;

  ARM_CT_UNROLL
  for (k = 0U; k < nbAcc; k++)
  {
    acc[k] = ARM_CT_FN(arm_ct_vzero_)();
  }

#if ARM_CT_VECTOR_EXT
  /* The windows are reloaded for each group : once the loop is unrolled,
     the compiler removes the redundant loads. Keeping the windows in an
     array from one iteration to the next one would force the array in
     memory when the loop is not fully unrolled. */
  if (numTaps <= ARM_CT_FULL_UNROLL_MAX_TAPS)
  {
    ARM_CT_UNROLL
    for (; t + ARM_CT_VL < numTaps; t += ARM_CT_VL)
    {
      ARM_CT_FN(arm_ct_fir_vec_group_)(acc, px + t, pb + t * stride, stride, nbAcc);
    }
  }
  else
  {
    ARM_CT_UNROLL_BY_4
    for (; t + ARM_CT_VL < numTaps; t += ARM_CT_VL)
    {
      ARM_CT_FN(arm_ct_fir_vec_group_)(acc, px + t, pb + t * stride, stride, nbAcc);
    }
  }
#endif

  /* Remaining taps (all the taps when ARM_CT_VECTOR_EXT is not set) */
  for (; t < numTaps; t++)
  {
    const ARM_CT_DT c = pb[t * stride];

    ARM_CT_UNROLL
    for (k = 0U; k < nbAcc; k++)
    {
      acc[k] += ARM_CT_VLD(px + t + k * ARM_CT_VL) * ARM_CT_FN(arm_ct_vdup_)(c);
    }
  }

  if (dstStride == 1U)
  {
    ARM_CT_UNROLL
    for (k = 0U; k < nbAcc; k++)
    {
      ARM_CT_VST(pDst + k * ARM_CT_VL, acc[k]);
    }
  }
  else
  {
    ARM_CT_UNROLL
    for (k = 0U; k < nbAcc; k++)
    {
      ARM_CT_UNROLL
      for (v = 0U; v < ARM_CT_VL; v++)
      {
        pDst[(k * ARM_CT_VL + v) * dstStride] = acc[k][v];
      }
    }
  }
}

#endif /* ARM_CT_TEMPLATE_VECTOR */

/**
  @brief         Forward copy of samples.
  @param[out]    pDst     points to the destination
  @param[in]     pSrc     points to the source
  @param[in]     nb       number of samples (compile-time constant)

  The buffers may overlap when pDst <= pSrc (copy of the last samples to the
  start of the state buffer) : each vector is read before being written and
  the following vectors are read at addresses not yet written.
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_ct_copy_)(
        ARM_CT_DT * pDst,
  const ARM_CT_DT * pSrc,
        uint32_t nb)
{
  uint32_t i = 0U;

#if ARM_CT_TEMPLATE_VECTOR
  ARM_CT_UNROLL
  for (; i + ARM_CT_VL <= nb; i += ARM_CT_VL)
  {
    ARM_CT_VST(pDst + i, ARM_CT_VLD(pSrc + i));
  }
#endif

  ARM_CT_UNROLL
  for (; i < nb; i++)
  {
    pDst[i] = pSrc[i];
  }
}

/**
  @brief         FIR kernel.
  @param[in,out] pState     points to the state buffer
  @param[in]     pCoeffs    points to the coefficients
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @param[in]     numTaps    number of taps (compile-time constant)
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_fir_ct_kernel_)(
        ARM_CT_DT * ARM_CT_RESTRICT_STATE pState,
  const ARM_CT_DT * ARM_CT_RESTRICT_COEFFS pCoeffs,
  const ARM_CT_DT * pSrc,
        ARM_CT_DT * pDst,
        uint32_t blockSize,
        uint32_t numTaps)
{
#if ARM_CT_TEMPLATE_VECTOR
  const uint32_t nbOut = ARM_CT_VK * ARM_CT_VL;
#else
  const uint32_t nbOut = ARM_CT_NB_ACC;
#endif
  ARM_CT_DT *pStateCur = pState + (numTaps - 1U);   /* Where the new samples are written */
  ARM_CT_DT *px = pState;                           /* First sample of the current output */
  uint32_t blkCnt;

  /* Blocks of nbOut outputs */
  blkCnt = blockSize / nbOut;

  while (blkCnt > 0U)
  {
    /* Copy the new input samples into the state buffer */
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, nbOut);
    pStateCur += nbOut;
    pSrc += nbOut;

#if ARM_CT_TEMPLATE_VECTOR
    ARM_CT_FN(arm_ct_fir_vec_block_)(px, pCoeffs, pDst, numTaps, 1U, 1U, ARM_CT_VK);
#else
    ARM_CT_FN(arm_ct_fir_block_)(px, pCoeffs, pDst, numTaps, 1U, 1U);
#endif

    pDst += nbOut;
    px += nbOut;

    /* Decrement loop counter */
    blkCnt--;
  }

#if ARM_CT_TEMPLATE_VECTOR
  /* Remaining outputs : blocks of VL outputs with one vector accumulator */
  blkCnt = (blockSize % nbOut) / ARM_CT_VL;

  while (blkCnt > 0U)
  {
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, ARM_CT_VL);
    pStateCur += ARM_CT_VL;
    pSrc += ARM_CT_VL;

    ARM_CT_FN(arm_ct_fir_vec_block_)(px, pCoeffs, pDst, numTaps, 1U, 1U, 1U);

    pDst += ARM_CT_VL;
    px += ARM_CT_VL;

    /* Decrement loop counter */
    blkCnt--;
  }

  blkCnt = blockSize % ARM_CT_VL;
#else
  blkCnt = blockSize % nbOut;
#endif

  /* Remaining outputs */
  while (blkCnt > 0U)
  {
    *pStateCur++ = *pSrc++;

    *pDst++ = ARM_CT_OUT(ARM_CT_FN(arm_ct_dot_)(px, pCoeffs, numTaps, 1U));

    px++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Keep the last numTaps - 1 samples for the next call */
  ARM_CT_FN(arm_ct_copy_)(pState, px, numTaps - 1U);
}

/**
  @brief         FIR decimator kernel.
  @param[in,out] pState     points to the state buffer
  @param[in]     pCoeffs    points to the coefficients
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of input samples to process
  @param[in]     numTaps    number of taps (compile-time constant)
  @param[in]     M          decimation factor (compile-time constant)
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_fir_decimate_ct_kernel_)(
        ARM_CT_DT * ARM_CT_RESTRICT_STATE pState,
  const ARM_CT_DT * ARM_CT_RESTRICT_COEFFS pCoeffs,
  const ARM_CT_DT * pSrc,
        ARM_CT_DT * pDst,
        uint32_t blockSize,
        uint32_t numTaps,
        uint32_t M)
{
  const uint32_t K = 4U;                            /* Outputs computed in parallel */
  ARM_CT_DT *pStateCur = pState + (numTaps - 1U);
  ARM_CT_DT *px = pState;
  uint32_t blkCnt, k, t;

  blkCnt = (blockSize / M) / K;

  while (blkCnt > 0U)
  {
    ARM_CT_AT acc[4];

    /* Copy K * M new input samples into the state buffer */
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, K * M);
    pStateCur += K * M;
    pSrc += K * M;

    t = 0U;

#if ARM_CT_TEMPLATE_VECTOR
    {
      ARM_CT_VEC accv[4], c;

      ARM_CT_UNROLL
      for (k = 0U; k < K; k++)
      {
        accv[k] = ARM_CT_FN(arm_ct_vzero_)();
      }

      for (; t + ARM_CT_VL <= numTaps; t += ARM_CT_VL)
      {
        c = ARM_CT_VLD(pCoeffs + t);

        ARM_CT_UNROLL
        for (k = 0U; k < K; k++)
        {
          accv[k] += ARM_CT_VLD(px + k * M + t) * c;
        }
      }

      /* Horizontal reductions */
      ARM_CT_UNROLL
      for (k = 0U; k < K; k++)
      {
        uint32_t v;

        acc[k] = 0;

        ARM_CT_UNROLL
        for (v = 0U; v < ARM_CT_VL; v++)
        {
          acc[k] += accv[k][v];
        }
      }
    }
#else
    ARM_CT_UNROLL
    for (k = 0U; k < K; k++)
    {
      acc[k] = 0;
    }
#endif

    /* Remaining taps (all the taps for the scalar version) */
    for (; t < numTaps; t++)
    {
      const ARM_CT_DT c = pCoeffs[t];

      ARM_CT_UNROLL
      for (k = 0U; k < K; k++)
      {
        acc[k] += (ARM_CT_AT) px[k * M + t] * c;
      }
    }

    ARM_CT_UNROLL
    for (k = 0U; k < K; k++)
    {
      pDst[k] = ARM_CT_OUT(acc[k]);
    }

    pDst += K;
    px += K * M;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Remaining outputs */
  blkCnt = (blockSize / M) % K;

  while (blkCnt > 0U)
  {
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, M);
    pStateCur += M;
    pSrc += M;

    *pDst++ = ARM_CT_OUT(ARM_CT_FN(arm_ct_dot_)(px, pCoeffs, numTaps, 1U));

    px += M;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Keep the last numTaps - 1 samples for the next call */
  ARM_CT_FN(arm_ct_copy_)(pState, px, numTaps - 1U);
}

/**
  @brief         FIR interpolator kernel.
  @param[in,out] pState     points to the state buffer
  @param[in]     pCoeffs    points to the coefficients
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of input samples to process
  @param[in]     phaseLen   length of each polyphase component (compile-time constant)
  @param[in]     L          interpolation factor (compile-time constant)
 */
__STATIC_FORCEINLINE void ARM_CT_FN(arm_fir_interpolate_ct_kernel_)(
        ARM_CT_DT * ARM_CT_RESTRICT_STATE pState,
  const ARM_CT_DT * ARM_CT_RESTRICT_COEFFS pCoeffs,
  const ARM_CT_DT * pSrc,
        ARM_CT_DT * pDst,
        uint32_t blockSize,
        uint32_t phaseLen,
        uint32_t L)
{
#if ARM_CT_TEMPLATE_VECTOR
  const uint32_t nbIn = ARM_CT_VK * ARM_CT_VL;
#else
  const uint32_t nbIn = ARM_CT_NB_ACC;
#endif
  ARM_CT_DT *pStateCur = pState + (phaseLen - 1U);
  ARM_CT_DT *px = pState;
  uint32_t blkCnt, j;

  /* Blocks of nbIn input samples (nbIn * L outputs) */
  blkCnt = blockSize / nbIn;

  while (blkCnt > 0U)
  {
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, nbIn);
    pStateCur += nbIn;
    pSrc += nbIn;

    /* One polyphase component for each output phase */
    for (j = 0U; j < L; j++)
    {
#if ARM_CT_TEMPLATE_VECTOR
      ARM_CT_FN(arm_ct_fir_vec_block_)(px, pCoeffs + (L - 1U - j), pDst + j, phaseLen, L, L, ARM_CT_VK);
#else
      ARM_CT_FN(arm_ct_fir_block_)(px, pCoeffs + (L - 1U - j), pDst + j, phaseLen, L, L);
#endif
    }

    pDst += nbIn * L;
    px += nbIn;

    /* Decrement loop counter */
    blkCnt--;
  }

#if ARM_CT_TEMPLATE_VECTOR
  /* Remaining input samples : blocks of VL samples with one vector accumulator */
  blkCnt = (blockSize % nbIn) / ARM_CT_VL;

  while (blkCnt > 0U)
  {
    ARM_CT_FN(arm_ct_copy_)(pStateCur, pSrc, ARM_CT_VL);
    pStateCur += ARM_CT_VL;
    pSrc += ARM_CT_VL;

    for (j = 0U; j < L; j++)
    {
      ARM_CT_FN(arm_ct_fir_vec_block_)(px, pCoeffs + (L - 1U - j), pDst + j, phaseLen, L, L, 1U);
    }

    pDst += ARM_CT_VL * L;
    px += ARM_CT_VL;

    /* Decrement loop counter */
    blkCnt--;
  }

  blkCnt = blockSize % ARM_CT_VL;
#else
  blkCnt = blockSize % nbIn;
#endif

  /* Remaining input samples */
  while (blkCnt > 0U)
  {
    *pStateCur++ = *pSrc++;

    for (j = 0U; j < L; j++)
    {
      *pDst++ = ARM_CT_OUT(ARM_CT_FN(arm_ct_dot_)(px, pCoeffs + (L - 1U - j), phaseLen, L));
    }

    px++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Keep the last phaseLen - 1 samples for the next call */
  ARM_CT_FN(arm_ct_copy_)(pState, px, phaseLen - 1U);
}

#if ARM_CT_TEMPLATE_VECTOR
#undef ARM_CT_VEC
#undef ARM_CT_VEC_MEM
#undef ARM_CT_VL
#undef ARM_CT_VK
#undef ARM_CT_VLD
#undef ARM_CT_VST
#undef ARM_CT_VLEN
#endif

#undef ARM_CT_DT
#undef ARM_CT_AT
#undef ARM_CT_OUT
#undef ARM_CT_NB_ACC
#undef ARM_CT_FN

#endif /* !defined(ARM_CT_TEMPLATE_T) */
