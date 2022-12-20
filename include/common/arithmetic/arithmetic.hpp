//
// Created by DELL on 2022/12/16.
//

#ifndef GAOFS_ARITHMETIC_HPP
#define GAOFS_ARITHMETIC_HPP

#include <cstdint>
#include <unistd.h>
#include <cassert>
// 一些用于计算的函数

namespace gaofs::utils::arithmetic {

 /**
 * 查看一个64位整数n是不是2的幂
 * @param [in] 整数n
 * @return n是2的幂的话就返回true，否则返回false
 */
 constexpr bool is_power_of_2(uint64_t n) {
    return n && (!(n & (n - 1u)));
 }

/**
 * 计算以2为底的对数
 * @param [in] 整数n
 * @return 以2为底的对数
 */
constexpr std::size_t log2(uint64_t n) {
    return 8u * sizeof(uint64_t) - __builtin_clzll(n) - 1;
}

/**
 * 计算n能否被块大小整除，也就是判断是否对齐
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 整数n
 * @param [in] block_size 块大小
 * @return 如果可以就为true
 */
constexpr bool is_aligned(const uint64_t n, const size_t block_size) {
     using gaofs::utils::arithmetic::log2;
     assert(is_power_of_2(block_size));
     return !(n & ((1u << log2(block_size)) - 1));
}

/**
 * 给定文件 @offset 和 @block_size，将 @offset 对齐到其最接近的左侧块边界。也就是低位清0
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @return 左对齐后的偏移
 */
constexpr uint64_t align_left(const uint64_t offset, const size_t block_size) {
     // This check is automatically removed in release builds
     assert(is_power_of_2(block_size));
     return offset & ~(block_size - 1u);
}

/**
 * 给定文件 @offset 和 @block_size，将 @offset 对齐到其最接近的右侧块边界。
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @return 右对齐后的偏移
 */
constexpr uint64_t align_right(const uint64_t offset, const size_t block_size) {
     // This check is automatically removed in release builds
     assert(is_power_of_2(block_size));
     return align_left(offset, block_size) + block_size;
}

/**
 * 返回将 @offset 与最接近的左侧块边界分隔开的溢出字节。 也就是比左边多多少
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @return 超出左侧块边界的大小
 */
constexpr size_t block_overrun(const uint64_t offset, const size_t block_size) {
    // This check is automatically removed in release builds
    assert(is_power_of_2(block_size));
    return offset & (block_size - 1u);
}

/**
 * 返回将 @offset 与最接近的右侧块边界还差多少。 也就是比右边少多少
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @return 还差右侧块边界的大小
 */
constexpr size_t block_underrun(const uint64_t offset, const size_t block_size) {
    // This check is automatically removed in release builds
    assert(is_power_of_2(block_size));
    return align_right(offset, block_size) - offset;
}

/**
 * 计算 @offset 对应的块索引
 * @note 这个函数假设 @block_size 是2的幂
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @return @offset 对应的块索引
 */
constexpr uint64_t block_index(const uint64_t offset, const size_t block_size) {
    using gaofs::utils::arithmetic::log2;
    using gaofs::utils::arithmetic::align_left;
    // This check is automatically removed in release builds
    assert(is_power_of_2(block_size));
    return align_left(offset, block_size) >> log2(block_size);
}

/**
 * 计算涉及的操作影响的块数
 * @note 这个函数假设 @block_size 是2的幂
 * @note 这个函数假设 @offset + @size 不会溢出
 *
 * @param [in] 偏移offset
 * @param [in] block_size 块大小
 * @param [in] 操作涉及的总大小size
 * @return 涉及的操作影响的块数
 */
constexpr std::size_t block_count(const uint64_t offset, const size_t size, const size_t block_size) {
    using gaofs::utils::arithmetic::log2;

    // These checks are automatically removed in release builds
    assert(is_power_of_2(block_size));

#if defined(__GNUC__) && !defined(__clang__)
    assert(!__builtin_add_overflow_p(offset, size, uint64_t{0}));
#else
    assert(offset + size > offset);
#endif

    const uint64_t first_block = align_left(offset, block_size);
    const uint64_t final_block = align_left(offset + size, block_size);
    const size_t mask = -!!size; // this is either 0 or ~0

    return (((final_block >> log2(block_size)) -
    (first_block >> log2(block_size)) +
    !is_aligned(offset + size, block_size))) &
    mask;
}

} // namespace gaofs::utils::arithmetic

#endif //GAOFS_ARITHMETIC_HPP
