#ifndef TATAMI_COMPARE_HELPERS_H
#define TATAMI_COMPARE_HELPERS_H

#include <limits>

/**
 * @file compare_helpers.hpp
 *
 * @brief Helper classes for comparison operations.
 * 
 * Classes defined here should be used as the `OP` in the `DelayedUnaryIsometricOp` class.
 */

namespace tatami {

/**
 * Type of the delayed comparison operation.
 */
enum class DelayedCompareOp : char { 
    EQUAL, 
    GREATER_THAN, 
    LESS_THAN, 
    GREATER_THAN_OR_EQUAL, 
    LESS_THAN_OR_EQUAL, 
    NOT_EQUAL
};

/**
 * @cond
 */
template<DelayedCompareOp op_, typename Value1_, typename Value2_>
bool delayed_compare_op(Value1_ val, Value2_ scalar) {
    if constexpr(op_ == DelayedCompareOp::EQUAL) {
        return val == scalar; 
    } else if constexpr(op_ == DelayedCompareOp::GREATER_THAN) {
        return val > scalar; 
    } else if constexpr(op_ == DelayedCompareOp::LESS_THAN) {
        return val < scalar; 
    } else if constexpr(op_ == DelayedCompareOp::GREATER_THAN_OR_EQUAL) {
        return val >= scalar; 
    } else if constexpr(op_ == DelayedCompareOp::LESS_THAN_OR_EQUAL) {
        return val <= scalar; 
    } else {
        return val != scalar; 
    }
}
/**
 * @endcond
 */

/**
 * @brief Compare a scalar to all values of a matrix.
 *
 * This should be used as the `OP` in the `DelayedUnaryIsometricOp` class.
 *
 * @tparam op_ Type of the comparison operation.
 * @tparam Value_ Type of value to be compared.
 */
template<DelayedCompareOp op_, typename Value_ = double>
struct DelayedCompareScalarHelper {
    /**
     * @param s Scalar value to be compared to.
     */
    DelayedCompareScalarHelper(Value_ s) : scalar(s) {}

    /**
     * Coordinates are ignored here and are only listed for compatibility purposes.
     *
     * @param r Row index, ignored.
     * @param c Column index, ignored.
     * @param val Matrix value to be compared to the scalar.
     *
     * @return Comparison result.
     */
    bool operator()(size_t r, size_t c, Value_ val) const { 
        return delayed_compare_op<op_>(val, scalar);
    }

    /**
     * Comparision is always assumed to discard structural sparsity, even when the scalar is non-zero.
     * NEED TO FIX THIS.
     */
    static const bool sparse_ = false;

    /**
     * This does not require row indices.
     */
    static const bool needs_row_ = false;

    /**
     * This does not require column indices.
     */
    static const bool needs_column_= false;

private:
    const Value_ scalar;
};

/**
 * @brief Compare vector elements along the rows or columns of a matrix.
 *
 * This should be used as the `OP` in the `DelayedUnaryIsometricOp` class.
 *
 * @tparam op_ Type of the comparison operation.
 * @tparam margin_ Dimension along which the addition is to occur.
 * If 0, each element of the vector is assumed to correspond to a row, and the same value is added to all entries in the same row of the matrix.
 * If 1, each element of the vector is assumed to correspond to a column instead.
 * @tparam Vector_ Class of the vector holding the values to be compared.
 */
template<DelayedCompareOp op_, int margin_, class Vector_>
struct DelayedCompareVectorHelper {
    /**
     * @param v Vector of values to be added.
     * This should be of length equal to the number of rows in the matrix if `margin_ = 0`, otherwise it should be of length equal to the number of columns.
     */
    DelayedCompareVectorHelper(Vector_ v) : vec(std::move(v)) {}

    /**
     * @param r Row index.
     * @param c Column index.
     * @param val Matrix value to be added.
     *
     * @return `val` plus the vector element at `r` (if `margin_ = 0`) or at `c` (if `margin_ = 1`).
     */
    template<typename Value_>
    bool operator()(size_t r, size_t c, Value_ val) const { 
        if constexpr(margin_ == 0) {
            return delayed_compare_op<op_>(val, vec[r]);
        } else {
            return delayed_compare_op<op_>(val, vec[c]);
        }
    }

    /**
     * Comparision is always assumed to discard structural sparsity, even when the vector elements are all non-zero.
     * NEED TO FIX THIS.
     */
    static const bool sparse_ = false; 

    /**
     * This requires row indices if `margin_ = 0`.
     */
    static const bool needs_row_ = margin_ == 0;

    /**
     * This requires column indices if `margin_ = 1`.
     */
    static const bool needs_column_ = margin_ == 1;

private:
    const Vector_ vec;
};

}

#endif
