#include <gtest/gtest.h>

#include <vector>

#ifdef CUSTOM_PARALLEL_TEST
// Put this before any tatami apply imports.
#include "custom_parallel.h"
#endif

#include "tatami/base/DenseMatrix.hpp"
#include "tatami/utils/convert_to_dense.hpp"
#include "tatami/utils/convert_to_sparse.hpp"
#include "tatami/stats/variances.hpp"

#include "../_tests/test_column_access.h"
#include "../_tests/test_row_access.h"
#include "../_tests/simulate_vector.h"

template<class L, class R>
void compare_double_vectors (const L& left, const R& right) {
    ASSERT_EQ(left.size(), right.size());
    for (size_t i = 0; i < left.size(); ++i) {
        EXPECT_FLOAT_EQ(left[i], right[i]);
    }
    return;
}

TEST(ComputingDimVariances, RowVariances) {
    size_t NR = 109, NC = 82;
    auto dump = simulate_sparse_vector<double>(NR * NC, 0.1);
    auto dense_row = std::unique_ptr<tatami::NumericMatrix>(new tatami::DenseRowMatrix<double>(NR, NC, dump));
    auto dense_column = tatami::convert_to_dense<false>(dense_row.get());
    auto sparse_row = tatami::convert_to_sparse<true>(dense_row.get());
    auto sparse_column = tatami::convert_to_sparse<false>(dense_row.get());

    // Doing the difference of squares as a quick-and-dirty reference.
    std::vector<double> ref(NR), expectedm(NR);
    for (size_t r = 0; r < NR; ++r) {
        for (size_t c = 0; c < NC; ++c) {
            double x = dump[c + r * NC];
            expectedm[r] += x;
            ref[r] += x * x;
        }
        expectedm[r] /= NC;
        ref[r] /= NC;
        ref[r] -= expectedm[r] * expectedm[r];
        ref[r] *= NC;
        ref[r] /= NC - 1;
    }

    compare_double_vectors(ref, tatami::row_variances(dense_row.get()));
    compare_double_vectors(ref, tatami::row_variances(dense_column.get()));
    compare_double_vectors(ref, tatami::row_variances(sparse_row.get()));
    compare_double_vectors(ref, tatami::row_variances(sparse_column.get()));

    // Same results from parallel code.
    compare_double_vectors(ref, tatami::row_variances(dense_row.get(), 3));
    compare_double_vectors(ref, tatami::row_variances(dense_column.get(), 3));
    compare_double_vectors(ref, tatami::row_variances(sparse_row.get(), 3));
    compare_double_vectors(ref, tatami::row_variances(sparse_column.get(), 3));
}

TEST(ComputingDimVariances, ColumnVariances) {
    size_t NR = 99, NC = 92;
    auto dump = simulate_sparse_vector<double>(NR * NC, 0.1);
    auto dense_row = std::unique_ptr<tatami::NumericMatrix>(new tatami::DenseRowMatrix<double>(NR, NC, dump));
    auto dense_column = tatami::convert_to_dense<false>(dense_row.get());
    auto sparse_row = tatami::convert_to_sparse<true>(dense_row.get());
    auto sparse_column = tatami::convert_to_sparse<false>(dense_row.get());

    // Doing the difference of squares as a quick-and-dirty reference.
    std::vector<double> ref(NC), expectedm(NC);
    for (size_t c = 0; c < NC; ++c) {
        for (size_t r = 0; r < NR; ++r) {
            double x = dump[c + r * NC];
            expectedm[c] += x;
            ref[c] += x * x;
        }
        expectedm[c] /= NR;
        ref[c] /= NR;
        ref[c] -= expectedm[c] * expectedm[c];
        ref[c] *= NR;
        ref[c] /= NR - 1;
    }

    compare_double_vectors(ref, tatami::column_variances(dense_row.get()));
    compare_double_vectors(ref, tatami::column_variances(dense_column.get()));
    compare_double_vectors(ref, tatami::column_variances(sparse_row.get()));
    compare_double_vectors(ref, tatami::column_variances(sparse_column.get()));

    // Same results from parallel code.
    compare_double_vectors(ref, tatami::column_variances(dense_row.get(), 3));
    compare_double_vectors(ref, tatami::column_variances(dense_column.get(), 3));
    compare_double_vectors(ref, tatami::column_variances(sparse_row.get(), 3));
    compare_double_vectors(ref, tatami::column_variances(sparse_column.get(), 3));
}

TEST(ComputingDimVariances, RowVariancesNaN) {
    auto dense = std::unique_ptr<tatami::NumericMatrix>(new tatami::DenseRowMatrix<double>(111, 0, std::vector<double>()));

    auto cref = tatami::column_variances(dense.get());
    EXPECT_EQ(cref.size(), 0);
    
    auto rref = tatami::row_variances(dense.get());
    EXPECT_TRUE(rref.size() > 0);
    EXPECT_TRUE(std::isnan(rref.front()));
    EXPECT_TRUE(std::isnan(rref.back()));
}

TEST(ComputingDimVariances, Configuration) {
    typedef tatami::stats::VarianceFactory<double> VarFact;
    EXPECT_TRUE(tatami::stats::has_sparse_running<VarFact>::value);
    EXPECT_TRUE(tatami::stats::has_sparse_running_parallel<VarFact>::value);
    EXPECT_TRUE(tatami::stats::has_dense_running<VarFact>::value);
    EXPECT_TRUE(tatami::stats::has_dense_running_parallel<VarFact>::value);
    EXPECT_TRUE(tatami::stats::has_sparse_direct<VarFact>::value);

    typedef decltype(std::declval<VarFact>().dense_direct()) VarDense;
    const bool ndc = tatami::stats::has_nonconst_dense_compute<VarDense, double, int>::value;
    EXPECT_FALSE(ndc);
    typedef decltype(std::declval<VarFact>().sparse_direct()) VarSparse;
    const bool nsc = tatami::stats::has_nonconst_sparse_compute<VarSparse, double, int>::value;
    EXPECT_FALSE(nsc);
    const tatami::SparseCopyMode nscc = tatami::stats::nonconst_sparse_compute_copy_mode<VarSparse>::value;
    EXPECT_EQ(nscc, tatami::SPARSE_COPY_BOTH); // just a negative control.
}

TEST(RunningVariances, SensibleZeros) {
    size_t NR = 55, NC = 52;
    auto dense_row = std::unique_ptr<tatami::NumericMatrix>(new tatami::DenseRowMatrix<double>(NR, NC, simulate_sparse_vector<double>(NR * NC, 0.1)));
    auto sparse_column = tatami::convert_to_sparse<false>(dense_row.get());

    // We force the first (non-zero) value to be zero, and we check that 
    // the number of non-zeros is still correctly reported.
    std::vector<double> running_vars(NR);
    std::vector<double> running_means(NR);
    std::vector<int> running_nzeros(NR);
    {
        std::vector<int> ibuffer(NR);
        std::vector<double> vbuffer(NR);
        std::vector<int> ref_nzeros(NR);
        auto wrk = sparse_column->new_column_workspace();

        for (int c = 0; c < static_cast<int>(NC); ++c) {
            auto range = sparse_column->sparse_column_copy(c, vbuffer.data(), ibuffer.data(), wrk.get(), tatami::SPARSE_COPY_VALUE);
            vbuffer[0] = 0; 
            tatami::stats::variances::compute_running(range, running_means.data(), running_vars.data(), running_nzeros.data(), c);
            for (size_t r = 1; r < range.number; ++r) {
                ref_nzeros[range.index[r]] += (range.value[r] != 0);
            }
        }
        tatami::stats::variances::finish_running(NR, running_means.data(), running_vars.data(), running_nzeros.data(), NC);

        EXPECT_EQ(ref_nzeros, running_nzeros);
    }

    // Unless we set skip_zeros = false.
    std::vector<double> running_vars2(NR);
    std::vector<double> running_means2(NR);
    std::vector<int> running_nzeros2(NR);
    {
        std::vector<int> ibuffer(NR);
        std::vector<double> vbuffer(NR);
        std::vector<int> ref_nzeros(NR);
        auto wrk = sparse_column->new_column_workspace();

        for (int c = 0; c < static_cast<int>(NC); ++c) {
            auto range = sparse_column->sparse_column_copy(c, vbuffer.data(), ibuffer.data(), wrk.get(), tatami::SPARSE_COPY_VALUE);
            vbuffer[0] = 0; 
            tatami::stats::variances::compute_running(range, running_means2.data(), running_vars2.data(), running_nzeros2.data(), c, false);
            for (size_t r = 0; r < range.number; ++r) {
                ++ref_nzeros[range.index[r]];
            }
        }
        tatami::stats::variances::finish_running(NR, running_means2.data(), running_vars2.data(), running_nzeros2.data(), NC);

        EXPECT_EQ(ref_nzeros, running_nzeros2);
        for (size_t i = 0; i < NR; ++i) {
            EXPECT_FLOAT_EQ(running_means2[i], running_means[i]);
            EXPECT_FLOAT_EQ(running_vars2[i], running_vars[i]);
        }
    }
}
