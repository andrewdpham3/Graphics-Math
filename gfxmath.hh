///////////////////////////////////////////////////////////////////////////////
// gfxmath.hh
//
// Linear algebra for graphics. This header includes two classes,
// gfx::vector that represents a low-dimensional vector, and
// gfx::matrix that represents a low-dimensional matrix. These classes
// are intended specifically for computer graphics applications so
// make some simplifying assumptions:
//
// - The dimensions of each vector/matrix is known at compile-time,
//   and is expressed as a template parameter.
//
// - Numerical: the elements of a vector/matrix may only be numeric
//   types, most likely int, float, or double. These classes do not
//   support vectors/matrices whose elements are non-numeric, such as
//   a vector of strings or matrix of matrices, even though those
//   concepts may be well-defined mathematically. In particular, this
//   code assumes that an element may be passed and returned by value,
//   and initialized to literal constants 0 and 1.
//
// - Low-dimension: vectors/matrices are only expected to be 4D or
//   smaller, so the largest anticipated data structure is a 4x4
//   matrix of doubles, occupying only 64 bytes. Therefore it is
//   reasonable to pass and return vectors/matrices by value, and
//   store them in stack-allocated arrays.
//
// - Further, since the dimension is small, Cramer's rule is an
//   acceptable algorithm for solving linear systems.
//
// Coding conventions:
//
// - All this code resides inside the gfx:: namespace, short for
//   "graphics".
//
// - Template parameters use the same order as std::array. So a vector
//   is declared as gfx::vector<scalar_type, dimension> and a matrix
//   is declared as gfx::matrix<scalar_type, height, width>.
//
// - "Vector" is the proper name for a mathematical vector, so we are
//   using it, even though it clashes with std::vector. Use
//   std::vector for teh STL data structure and gfx::vector for the
//   linear-algebra vector type.
//
// - Using-declarations are used to create concise aliases for long
//   types. There are aliases for vector2, vector3, vector4, and
//   matrix2x2, matrix3x3, and matrix4x4.
//
// - Since vector/matrix dimensions are template parameters, we use
//   types aggressively to ensure that math expressions are
//   well-typed, and fail early at compile-time when they are not. For
//   example the matrix * operator insists that the matrix operands
//   have compatible dimensions, so incompatible multiplies trigger
//   compile errors.
//
// - In operator overloads, "lhs" refers to the left-hand-side
//   operand, and "rhs" refers to the right-hand-side operand.
//
// - Function preconditions are checked with assertions. Compile-time
//   assertions with static_assert() are preferred over run-time
//   assertions with assert(), to catch errors as early as possible.
//
// - Default constructors initalize all vector/matrix elements to zero.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <memory>
#include <ostream>

namespace gfx {

    const double DEFAULT_DELTA = .001;

    // Return true when lhs and rhs are approximately equal. This
    // function is intended to compare floating points values for
    // equality, while ignoring small rounding errors. delta is the
    // maximum error, as a fraction of rhs, that counts as equal. delta
    // must be positive. Ordinarily you want delta to be a small postive
    // fraction, such as .001. This function handles positive and
        // negative infinity properly.
        template <typename scalar_type>
      bool almost_equal(scalar_type lhs,
            scalar_type rhs,
            double delta) {
        assert(delta > 0.0);
        if (lhs == rhs) {
        // Handle values considered truly equal according to ==, as well
        // as infinite values.
        return true;
        } else {
        // Use the delta calculation, using absolute values to be
        // careful about negative values.
        double lhs_double = static_cast<double>(lhs),
            rhs_double = static_cast<double>(rhs),
            signed_difference = lhs_double - rhs_double,
            positive_difference = fabs(signed_difference);
        assert(positive_difference >= 0.0);
        return (positive_difference <= delta);
        }
      }


    // Class for a mathematical vector with DIMENSION elements, each of
    // type scalar_type.
    template <typename scalar_type, int DIMENSION>
    class vector {
    public:

        using same_type = gfx::vector<scalar_type, DIMENSION>;

        static bool is_index(int i) {
            return ((i >= 0) && (i < DIMENSION));
        }

        vector(scalar_type default_value = 0) {
            fill(default_value);
        }

        // Initializer list constructor. You can use this with something like
        //
        // gfx::vector<double, 3> v({1.0, 2.0, 3.0});
        //
        // to initialize v with the values <1, 2, 3>.
        //
        // If you provide fewer values than the vector contains, the
        // unmentioned elements are initialized to zero. If you provide
        // more values, the extras are ignored.
        vector(std::initializer_list<scalar_type> il) {
            auto it = il.begin();
            for (int i = 0; i < DIMENSION; ++i) {
                if (it != il.end()) {
                    _elements[i] = *it;
                    //cout << "added" << _elements[i];
                    it++;
                }
                else {
                    _elements[i] = 0;
                }
            }
        }

        vector(const same_type& init)
            : _elements(init._elements) { }

        same_type& operator=(const same_type& rhs) {
            _elements = rhs._elements;
            return *this;
        }

        bool operator==(const same_type& rhs) const {
            int i = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                if (_elements[i] != rhs._elements[i])
                    return false;
                i++;
            }
            return true;
        }

        bool operator!=(const same_type& rhs) const {
            int i = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                if (_elements[i] != rhs._elements[i])
                    return true;
                i++;
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const same_type& rhs) {
            os << "<";
            for (int i = 0; i < DIMENSION; ++i) {
                if (i > 0) {
                    os << ", ";
                }
                os << rhs._elements[i];
            }
            os << ">";
            return os;
        }

        const scalar_type& operator[](int i) const {
            assert(is_index(i));
            return _elements[i];
        }

        scalar_type& operator[](int i) {
            assert(is_index(i));
            return _elements[i];
        }

        // Add a vector.
        same_type operator+(const same_type& rhs) const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] + rhs._elements[i];
                i++;
            }
            return ret;
        }

        // Subtract a vector.
        same_type operator-(const same_type& rhs) const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] - rhs._elements[i];
                i++;
            }
            return ret;
        }

        // Negate this vector.
        same_type operator-() const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] * -1;
                i++;
            }
            return ret;
        }

        // Multiply by a scalar.
        same_type operator*(scalar_type rhs) const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] * rhs;
                i++;
            }
            return ret;
        }

        // Multiply by a vector (dot product). Cross product is
        // implemented in the cross() function below.
        scalar_type operator*(const same_type& rhs) const {
            int i = 0;
            int sum = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                sum += _elements[i] * rhs._elements[i];
                i++;
            }
                return sum;
        }

        // Divide by a scalar.
        same_type operator/(scalar_type rhs) const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] / rhs;
                i++;
            }
            return ret;
        }

        // Return true when this vector is approximately equal to rhs,
        // according to the almost_equal function.
        bool almost_equal(const same_type& rhs, double delta = DEFAULT_DELTA) const {
            for (int i=0;i<_elements.size();i++)
                if (_elements[i]-rhs[i]>delta)
                    return false;
            return true;
        }

        int dimension() const {
            return DIMENSION;
        }

        // Set all elements to value.
        void fill(scalar_type value) {
            int i = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it){
                _elements[i] = value;
                i++;
            }
        }

        // The magnitude of this vector, squared (i.e. raised to the
        // second power). This function is provided because it should be
        // faster than magnitude(), and in some use cases
        // magnitude_squared() is sufficient.
        scalar_type magnitude_squared() const {
            int sos = 0;//sum of squares
            int i = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                sos += pow(_elements[i], 2);
                i++;
            }
            return sos;
        }

        // The magnitude of this vector, i.e. the square root of the sum
        // of the squares of the elements.
        scalar_type magnitude() const {
            int sos = 0;//sum of squares
            int i = 0;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                sos += pow(_elements[i],2);
                i++;
            }
            return sqrt(sos);
        }

        // Return a vector with the same direction as this vector, but
        // whose magnitude is normalized to 1.
        same_type normalize() const {
            int i = 0;
            auto ret = *this;
            for (auto it = _elements.begin(); it != _elements.end(); ++it) {
                ret[i] = _elements[i] / magnitude();
                //cout << _elements[i] / magnitude() << "|";
                i++;
            }
            return ret;
        }

        // Return the cross product this x rhs . Note, since in general
        // cross product is not commutative, that this matrix object is
        // the left operand. This operation is only defined for 3D
        // vectors.
        same_type cross(const same_type& rhs) const {
            auto ret = *this;
            ret[0] = (_elements[1] * rhs._elements[2]) - (_elements[2] * rhs._elements[1]);
            ret[1] = (_elements[2] * rhs._elements[0]) - (_elements[0] * rhs._elements[2]);
            ret[2] = (_elements[0] * rhs._elements[1]) - (_elements[1] * rhs._elements[0]);
            return ret;
        }

        // Return a portion of this vector, specifically RESULT_DIMENSION
        // elements starting from start_index. The result dimension cannot
        // be larger than this source vector, and the range of indices in
        // the subvector must actually exist in this vector.
        template<const int RESULT_DIMENSION>
        gfx::vector<scalar_type, RESULT_DIMENSION> subvector(int start_index) const {

            static_assert(RESULT_DIMENSION <= DIMENSION,
                "subvector cannot be larger than original vector");

            assert(is_index(start_index + RESULT_DIMENSION - 1));

            gfx::vector<scalar_type, RESULT_DIMENSION> v(0);

            for(int i=0;i<RESULT_DIMENSION;i++)
                v[i] = _elements[i + start_index];
            return v;
        }

        // Return a lower-dimension copy of this vector, which includes
        // only the first RESULT_DIMENSION elements.
        template<int RESULT_DIMENSION>
        gfx::vector<scalar_type, RESULT_DIMENSION> shrink() const {
            static_assert(RESULT_DIMENSION < DIMENSION,
                "shrink'd vector must be smaller");
            
            gfx::vector<scalar_type, RESULT_DIMENSION> v(0);

            for (int i = 0; i<RESULT_DIMENSION; i++)
                v[i] = _elements[i];
            return v;
        }

        // Return a higher-dimension copy of this vector. The
        // newly-created elements are all initialized to
        // default_value. The new dimension must be strictly greater than
        // this vector's dimension.
        template<int RESULT_DIMENSION>
        gfx::vector<scalar_type, RESULT_DIMENSION> grow(scalar_type default_value = 0) {

            static_assert(RESULT_DIMENSION > DIMENSION,
                "grown vector must be larger than original vector");

            gfx::vector<scalar_type, RESULT_DIMENSION> v(0);
            for (int i = 0; i < dimension(); i++) {
                v[i] = _elements[i];
            }
            for (int i = 1; i<RESULT_DIMENSION; i++)
                v[i] = default_value;
            return v;
        }

    private:

        std::array<scalar_type, DIMENSION> _elements;
    };

    // Aliases for vectors of 2, 3, and 4 dimensions.
    template <typename scalar_type> using vector2 = gfx::vector<scalar_type, 2>;
    template <typename scalar_type> using vector3 = gfx::vector<scalar_type, 3>;
    template <typename scalar_type> using vector4 = gfx::vector<scalar_type, 4>;

    // Class for a mathematical matrix with HEIGHT rows, WIDTH columns,
    // and each element is of type scalar_type. Each row is a
    // gfx::vector object.
    template <typename scalar_type,
        int HEIGHT,
        int WIDTH>
        class matrix {
        public:

            using same_type = gfx::matrix<scalar_type, HEIGHT, WIDTH>;
            using row_type = gfx::vector<scalar_type, WIDTH>;

            static constexpr bool is_row(int r) {
                return ((r >= 0) && (r < HEIGHT));
            }

            static constexpr bool is_column(int c) {
                return ((c >= 0) && (c < WIDTH));
            }

            // Return true when this matrix is square, i.e. its width and
            // height are identical.
            static constexpr bool is_square() {
                return (WIDTH == HEIGHT);
            }

            matrix(scalar_type default_value = 0) {
                fill(default_value);
            }

            matrix(const same_type& rhs)
                : _rows(rhs._rows) { }

            // Initializer list constructor. Elements are added in row-major
            // order, i.e. the first row is filled left-to-right, then the
            // second row, and so on. You can use this with something like
            //
            // gfx::matrix<double, 2, 2> m({1.0, 2.0, 3.0, 4.0});
            //
            // to initialize m with the values
            //    |1 2|
            //    |3 4| .
            //
            // If you provide fewer values than the matrix contains, the
            // unmentioned elements are initialized to zero. If you provide
            // more values, the extras are ignored.
            matrix(std::initializer_list<scalar_type> il) {
                auto it = il.begin();
                for (int i = 0; i < HEIGHT; ++i) {
                    for (int j = 0; j < WIDTH; ++j) {
                        if (it != il.end()) {
                            _rows[i][j] = *it;
                            it++;
                        }
                        else {
                            _rows[i][j] = 0;
                        }
                    }
                }
            }

            same_type& operator=(const same_type& rhs) {
                _rows = rhs._rows;
                return *this;
            }

            bool operator==(const same_type& rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return false;
            }

            bool operator!=(const same_type& rhs) const {
                return !(*this == rhs);
            }

            friend std::ostream& operator<<(std::ostream& os, const same_type& rhs) {
                for (int i = 0; i < HEIGHT; ++i) {
                    os << "|";
                    for (int j = 0; j < WIDTH; ++j) {
                        if (j > 0) {
                            os << " ";
                        }
                        os << rhs._rows[i][j];
                    }
                    os << "|" << std::endl;
                }
                return os;
            }

            const row_type& operator[](int row) const {
                assert(is_row(row));
                return _rows[row];
            }

            row_type& operator[](int row) {
                assert(is_row(row));
                return _rows[row];
            }

            // Add a matrix.
            same_type operator+(const same_type& rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return *this;
            }

            // Subtract a matrix.
            same_type operator-(const same_type& rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return *this;
            }

            // Negate this matrix.
            same_type operator-() const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return *this;
            }

            // Divide by a scalar.
            same_type operator/(scalar_type rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return *this;
            }

            // Multiply by a scalar.
            same_type operator*(scalar_type rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return *this;
            }

            // Multiply by a matrix.
            template<int RESULT_WIDTH>
            gfx::matrix<scalar_type, HEIGHT, RESULT_WIDTH>
                operator*(const gfx::matrix<scalar_type, WIDTH, RESULT_WIDTH>& rhs) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, HEIGHT, RESULT_WIDTH>();
            }

            // Return a portion of this matrix, of height RESULT_HEIGHT and
            // width RESULT_WIDTH, starting from the "top" row and "left"
            // column. The submatrix must actually fit inside this matrix.
            template<int RESULT_HEIGHT, int RESULT_WIDTH>
            gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH> submatrix(int top,
                int left) const {
                static_assert(RESULT_HEIGHT > 0,
                    "submatrix height must be positive");
                static_assert(RESULT_WIDTH > 0,
                    "submatrix width must be positive");
                static_assert(RESULT_HEIGHT <= HEIGHT,
                    "submatrix height must be less than source matrix height");
                static_assert(RESULT_WIDTH <= WIDTH,
                    "submatrix width must be less than source matrix width");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH>();
            }

            // Return a smaller version of this matrix, keeping only the first
            // RESULT_HEIGHT rows and RESULT_WIDTH columns. The resulting
            // matrix must be strictly smaller than this matrix.
            template<int RESULT_HEIGHT, int RESULT_WIDTH>
            gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH> shrink() const {
                static_assert(RESULT_HEIGHT <= HEIGHT,
                    "shrunk matrix height must be less than or equal to source matrix height");
                static_assert(RESULT_WIDTH <= WIDTH,
                    "shrunk matrix width must be less than or equal to source matrix width");
                static_assert((RESULT_WIDTH < WIDTH) || (RESULT_HEIGHT < HEIGHT),
                    "shrunk matrix must be strictly smaller than the source matrix");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH>();
            }

            // Return a larger version of this matrix. The newly-created
            // elements are all initialized to default_value. The new vector
            // must be larger than this vector.
            template<int RESULT_HEIGHT, int RESULT_WIDTH>
            gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH> grow(scalar_type default_value = 0) const {
                static_assert(RESULT_HEIGHT >= HEIGHT,
                    "enlarged matrix must be higher than source matrix");
                static_assert(RESULT_WIDTH >= WIDTH,
                    "enlarged matrix must be wider than source matrix");
                static_assert((RESULT_HEIGHT > HEIGHT) || (RESULT_WIDTH > WIDTH),
                    "enlarged matrix must be larger than source matrix");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, RESULT_HEIGHT, RESULT_WIDTH>();
            }

            // Return true when this matrix is approximately equal to rhs,
            // according to the almost_equal function.
            bool almost_equal(const same_type& rhs,
                double delta = DEFAULT_DELTA) const {
                for (int i = 0; i < HEIGHT; ++i) {
                    if (!_rows[i].almost_equal(rhs[i], delta)) {
                        return false;
                    }
                }
                return true;
            }

            // Return one column of this matrix as a matrix object.
            gfx::matrix<scalar_type, HEIGHT, 1> column_matrix(int column) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, HEIGHT, 1>();
            }

            // Return one column of this matrix as a vector object.
            gfx::vector<scalar_type, HEIGHT> column_vector(int column) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::vector<scalar_type, HEIGHT>();
            }

            // Assign all elements in this matrix to value.
            void fill(scalar_type value) {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
            }

            // Return the height of this matrix.
            static int height() {
                return HEIGHT;
            }

            // Return an identity matrix with the same dimensions as this
            // matrix, which must be square.
            static same_type identity() {
                static_assert(is_square(),
                    "identity matrix must be square");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return same_type();
            }

            // Return one row of this matrix as a matrix object.
            gfx::matrix<scalar_type, 1, WIDTH> row_matrix(int row) const {
                assert(is_row(row));
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, 1, WIDTH>();
            }

            // Return one row of this matrix as a vector object.
            row_type row_vector(int row) const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return row_type();
            }

            // Return the transposition of this matrix.
            gfx::matrix<scalar_type, WIDTH, HEIGHT> transpose() const {
                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::matrix<scalar_type, WIDTH, HEIGHT>();
            }

            // Return the width of this matrix.
            static int width() {
                return WIDTH;
            }

            // Return the determinant of this matrix. This function is only
            // implemented for square 2x2 and 3x3 matrices.
            scalar_type determinant() const {
                static_assert(is_square(),
                    "determinant is only defined for square matrices");
                static_assert((WIDTH == 2) || (WIDTH == 3),
                    "determinant only implemented for 2x2 and 3x3 matrices");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return 0;
            }

            // Solve a linear system, Ax=b, where this matrix contains the
            // coefficients A, and the passed-in vector b contains the
            // constants on the right-hand-side of the equations. This
            // function is only implemented for square 2x2 and 3x3
            // matrices. This function uses Cramer's rule, which is reasonably
            // efficient for matrices up to 4x4, but would be slow for large
            // matrices.
            gfx::vector<scalar_type, HEIGHT>
                solve(const gfx::vector<scalar_type, HEIGHT>& b) const {
                static_assert(is_square(),
                    "only square linear systems can be solved");
                static_assert((WIDTH == 2) || (WIDTH == 3),
                    "solve is only implemented for 2x2 and 3x3 matrices");

                // TODO: replace this function body with working code. Make sure
                // to delete this comment.
                return gfx::vector<scalar_type, HEIGHT>();
            }

        private:
            std::array<row_type, HEIGHT> _rows;
    };

    template <typename scalar_type> using matrix2x2 = gfx::matrix<scalar_type, 2, 2>;
    template <typename scalar_type> using matrix3x3 = gfx::matrix<scalar_type, 3, 3>;
    template <typename scalar_type> using matrix4x4 = gfx::matrix<scalar_type, 4, 4>;
}
