#ifndef AMICI_VECTOR_H
#define AMICI_VECTOR_H

#include <vector>
#include <type_traits>

#include <amici/exception.h>

#include <nvector/nvector_serial.h>

#include <gsl/gsl-lite.hpp>

namespace amici {

/** Since const N_Vector is not what we want */
using const_N_Vector =
    std::add_const<typename std::remove_pointer<N_Vector>::type *>::type;

/** AmiVector class provides a generic interface to the NVector_Serial struct */
class AmiVector {
  public:
    /**
     * @brief Default constructor
     */
    AmiVector() = default;

    /** Creates an std::vector<realtype> and attaches the
     * data pointer to a newly created N_Vector_Serial.
     * Using N_VMake_Serial ensures that the N_Vector
     * module does not try to deallocate the data vector
     * when calling N_VDestroy_Serial
     * @brief emmpty constructor
     * @param length number of elements in vector
     */
    explicit AmiVector(const long int length)
        : vec(static_cast<decltype(vec)::size_type>(length), 0.0),
          nvec(N_VMake_Serial(length, vec.data())) {}

    /** Moves data from std::vector and constructs an nvec that points to the
     * data
     * @brief constructor from std::vector,
     * @param rvec vector from which the data will be moved
     */
    explicit AmiVector(std::vector<realtype> rvec)
        : vec(std::move(rvec)),
          nvec(N_VMake_Serial(static_cast<long int>(vec.size()), vec.data())) {}

    /**
     * @brief copy constructor
     * @param vold vector from which the data will be copied
     */
    AmiVector(const AmiVector &vold) : vec(vold.vec) {
        nvec =
            N_VMake_Serial(static_cast<long int>(vold.vec.size()), vec.data());
    }

    /**
     * @brief copy assignment operator
     * @param other right hand side
     * @return left hand side
     */
    AmiVector &operator=(AmiVector const &other);

    /**
     * @brief data accessor
     * @return pointer to data array
     */
    realtype *data();

    /**
     * @brief const data accessor
     * @return const pointer to data array
     */
    const realtype *data() const;

    /**
     * @brief N_Vector accessor
     * @return N_Vector
     */
    N_Vector getNVector();

    /**
     * @brief N_Vector accessor
     * @return N_Vector
     */
    const_N_Vector getNVector() const;

    /**
     * @brief Vector accessor
     * @return Vector
     */
    std::vector<realtype> const &getVector();

    /**
     * @brief returns the length of the vector
     * @return length
     */
    int getLength() const;

    /**
     * @brief resets the Vector by filling with zero values
     */
    void reset();

    /**
     * @brief changes the sign of data elements
     */
    void minus();

    /**
     * @brief sets all data elements to a specific value
     * @param val value for data elements
     */
    void set(realtype val);

    /**
     * @brief accessor to data elements of the vector
     * @param pos index of element
     * @return element
     */
    realtype &operator[](int pos);
    /**
     * @brief accessor to data elements of the vector
     * @param pos index of element
     * @return element
     */
    realtype &at(int pos);

    /**
     * @brief accessor to data elements of the vector
     * @param pos index of element
     * @return element
     */
    const realtype &at(int pos) const;

    /**
     * @brief copies data from another AmiVector
     * @param other data source
     */
    void copy(const AmiVector &other);

    /**
     * @brief destructor
     */
    ~AmiVector();

  private:
    /** main data storage */
    std::vector<realtype> vec;

    /** N_Vector, will be synchronised such that it points to data in vec */
    N_Vector nvec = nullptr;

    /**
     * @brief reconstructs nvec such that data pointer points to vec data array
     */
    void synchroniseNVector();
};

/**
 * @brief AmiVectorArray class.
 *
 * Provides a generic interface to arrays of NVector_Serial structs
 */
class AmiVectorArray {
  public:
    /**
     * @brief Default constructor
     */
    AmiVectorArray() = default;

    /**
     * Creates an std::vector<realype> and attaches the
     * data pointer to a newly created N_VectorArray
     * using CloneVectorArrayEmpty ensures that the N_Vector
     * module does not try to deallocate the data vector
     * when calling N_VDestroyVectorArray_Serial
     * @brief empty constructor
     * @param length_inner length of vectors
     * @param length_outer number of vectors
     */
    AmiVectorArray(long int length_inner, long int length_outer);
    
    /**
     * @brief copy constructor
     * @param vaold object to copy from
     */
    AmiVectorArray(const AmiVectorArray &vaold);

    /**
     * @brief copy assignment operator
     * @param other right hand side
     * @return left hand side
     */
    AmiVectorArray &operator=(AmiVectorArray const &other);

    /**
     * @brief accessor to data of AmiVector elements
     * @param pos index of AmiVector
     * @return pointer to data array
     */
    realtype *data(int pos);

    /**
     * @brief const accessor to data of AmiVector elements
     * @param pos index of AmiVector
     * @return const pointer to data array
     */
    const realtype *data(int pos) const;

    /**
     * @brief accessor to elements of AmiVector elements
     * @param ipos inner index in AmiVector
     * @param jpos outer index in AmiVectorArray
     * @return element
     */
    realtype &at(int ipos, int jpos);

    /**
     * @brief const accessor to elements of AmiVector elements
     * @param ipos inner index in AmiVector
     * @param jpos outer index in AmiVectorArray
     * @return element
     */
    const realtype &at(int ipos, int jpos) const;

    /**
     * @brief accessor to NVectorArray
     * @return N_VectorArray
     */
    N_Vector *getNVectorArray();

    /**
     * @brief accessor to NVector element
     * @param pos index of corresponding AmiVector
     * @return N_Vector
     */
    N_Vector getNVector(int pos);

    /**
     * @brief const accessor to NVector element
     * @param pos index of corresponding AmiVector
     * @return N_Vector
     */
    const_N_Vector getNVector(int pos) const;

    /**
     * @brief accessor to AmiVector elements
     * @param pos index of AmiVector
     * @return AmiVector
     */
    AmiVector &operator[](int pos);

    /**
     * @brief const accessor to AmiVector elements
     * @param pos index of AmiVector
     * @return const AmiVector
     */
    const AmiVector &operator[](int pos) const;

    /**
     * @brief length of AmiVectorArray
     * @return length
     */
    int getLength() const;

    /**
     * @brief resets every AmiVector in AmiVectorArray
     */
    void reset();

    /**
     * @brief flattens the AmiVectorArray to a vector in row-major format
     * @param vec vector into which the AmiVectorArray will be flattened. Must
     * have length equal to number of elements.
     */
    void flatten_to_vector(std::vector<realtype> &vec) const;

    /**
     * @brief copies data from another AmiVectorArray
     * @param other data source
     */
    void copy(const AmiVectorArray &other);

    ~AmiVectorArray() = default;

  private:
    /** main data storage */
    std::vector<AmiVector> vec_array;

    /**
     * N_Vector array, will be synchronised such that it points to
     * respective elements in the vec_array
     */
    std::vector<N_Vector> nvec_array;
};

} // namespace amici


namespace gsl {
/**
 * @brief Create span from N_Vector
 * @param nv
 * @return
 */
inline span<realtype> make_span(N_Vector nv)
{
    return span<realtype>(N_VGetArrayPointer(nv), N_VGetLength_Serial(nv));
}
} // namespace gsl

#endif /* AMICI_VECTOR_H */
