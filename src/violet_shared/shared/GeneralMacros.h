/*******************************************************************
 *                       VioletOS v0.0.1
 *        Created by Ranyodh Singh Mandur - 💜 2026-2026
 *
 *             Licensed under the MIT License (MIT).
 *        For more details, see the LICENSE file or visit:
 *              https://opensource.org/licenses/MIT
 *
 *         VioletOS is a free open source operating system
********************************************************************/
#ifndef VIOLET_SHARED_GENERAL_MACROS_HG
#define VIOLET_SHARED_GENERAL_MACROS_HG

//================================================================================ Force Inline ================================================================================//

#if defined(_MSC_VER)
#   define VIOLET_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define VIOLET_FORCEINLINE inline __attribute__((always_inline))
#else
#   define VIOLET_FORCEINLINE inline // Fallback for anything else
#endif /*VIOLET_FORCEINLINE*/

//================================================================================ Likely/Unlikely ================================================================================//

#if defined(__GNUC__) || defined(__clang__)
#   define VIOLET_LIKELY(fp_Condition)   __builtin_expect(!!(fp_Condition), 1)
#   define VIOLET_UNLIKELY(fp_Condition) __builtin_expect(!!(fp_Condition), 0)
#else
#   define VIOLET_LIKELY(fp_Condition)   (fp_Condition) //just use PGO or whatever for MSVC owo
#   define VIOLET_UNLIKELY(fp_Condition) (fp_Condition)
#endif


//================================================================================ Compiler Barrier ================================================================================//

#define VIOLET_COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

//================================================================================ Halt ================================================================================//

#define VIOLET_HALT() __asm__ volatile ("hlt")

#endif /*VIOLET_SHARED_GENERAL_MACROS_HG*/