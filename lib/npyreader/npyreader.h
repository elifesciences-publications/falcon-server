/**
 * \file npyreader.h
 * \brief functions to read a .npy numpy file.
 * \author: Jordi Castells Sala
 *
 * This functions all follow the documentation in
 * https://github.com/numpy/numpy/blob/master/doc/neps/npy-format.txt
 *
 */
#ifndef NPY_UTILS_H 
#define NPY_UTILS_H

#include <inttypes.h>
#include <stdio.h>

/**
 * \brief constants to seek information in a standard .npy file
 */
#define MAGIC_SIZE 6
#define VERSION_SEEK 6
#define MINOR_VERSION_SEEK 7
#define HEADER_LEN_SEEK 8
#define DICT_SEEK 10

#define NPYERR_READDICT 1
#define NPYERR_GETSHAPE 2
#define NPYERR_BADSIZENUMBER 3
#define NPYERR_READDATA 4
#define NPYERR_ALLOCATEDATA 5
#define NPYERR_FREAD 6
#define NPYERR_FSEEK 7
#define NPYERR_NOMEM 8
#define NPYERR_SUBNOMEM 8
#define NPYERR_SIZETOOLONG 9

/**
 * \brief Type definitions of specific floats and bytes
 */
typedef float float32_t;
typedef double float64_t;
typedef long double float128_t;
typedef unsigned char byte;

/**
 * \brief Get the numpy Magic string. 
 *
 * The file should be a .npy opened file.
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \return A string containing the magic string. 
 * \retval NULL if an error ocurred
 */
char* retrieve_magic(FILE* fp);


/**
 * \brief Get the numpy major version number
 *
 * The user is responsible for freeing the memory
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval uint8_t with the major version number
 */
uint8_t retrieve_version_number_major(FILE* fp);

/**
 * \brief Get the numpy minor version number
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval uint8_t with the minor version number
 * \retval -1 if an error ocurred
 */
uint8_t retrieve_version_number_minor(FILE* fp);

/**
 * \brief Get the length of the HEADER
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval uint16_t with the length of the dictionary header.
 * \retval -1 if an error ocurred
 */
uint16_t retrieve_HEADER_LEN(FILE* fp);


/**
 * \brief Get the python-dictionary string from the .npy file
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval A string with the python-dictionary definition
 * \retval NULL if an error ocurred
 */
char* retrieve_python_dict(FILE* fp);


/**
 * \brief Retrieve data from the .npy file
 *
 * Gets the data from the .npy file starting just after the HEADER
 * finished and formats it as an array of float32_t values
 * \note The user is responsible of freeing the float32_t*
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval float32_t array with the extracted data.
 * \retval NULL if an error ocurred
 */
float32_t* retrieve_npy_float32(FILE* fp);

/**
 * \brief Retrieve data from the .npy file
 *
 * Gets the data from the .npy file starting just after the HEADER
 * finished and formats it as an array of float64_t valuesh.
 * \note The user is responsible of freeing the float64_t*
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval float64_t array with the extracted data.
 * \retval NULL if an error ocurred
 */
float64_t* retrieve_npy_float64(FILE* fp);

/**
 * \brief Retrieve data from the .npy file
 *
 * Gets the data from the .npy file starting just after the HEADER
 * finished and formats it as an array of int32_t values.
 *
 * \note The user is responsible of freeing the int32_t*
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval int32_t array with the extracted data.
 * \retval NULL if an error ocurred
 */
int32_t* retrieve_npy_int32(FILE* fp);



/**
 * \brief Creates a matrix sized as rows,cols with the values from data
 *
 * Gets the data from the .npy file starting just after the HEADER
 * finished and formats it as an array of float64_t values.
 * The functions does not check for the correctness of the matrix size.
 * It is expected to be invoked with the output values of:
 * retrieve_npy_float32 and get_shape
 *
 * \note The user is responsible of freeing the float32_t**
 *
 * \param[in] data float32_t array of values.
 * \param[in] rows number of rows to generate
 * \param[in] cols number of cols to generate.
 *
 * \retval float32_t matrix with the extracted data.
 * \retval NULL if error
 */
float32_t** allocate_matrix_f32(float32_t* data, int rows, int cols);


/**
 * \brief Creates a 2D matrix from the .npy file
 *
 * \note The user is responsible of freeing the float32_t**
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval float32_t matrix with the extracted data.
 * \retval NULL if an error ocurred
 */
float32_t** get_2D_matrix_f32(FILE* fp);

/**
 * \brief Creates a 1D array from the .npy file
 *
 * \note The user is responsible of freeing the float32_t*
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 *
 * \retval float32_t 1D array with the extracted data.
 * \retval NULL if an error ocurred
 */
float32_t* get_1D_array_f32(FILE* fp);

/**
 * \brief Gets the size of the sizes array
 *
 *
 * \param[in] sizes uint32_t array created by get_shape or get_shape_file
 *
 * \retval int with the dimensions of the matrix
 */
int get_number_of_sizes(uint32_t* sizes);


/**
 * \brief Get the dimensions of the array or matrix
 *
 * \note The user is responsible of freeing the uint32_t*
 *
 * \param[in] dictstring A string with a python-like dictionary.
 * \retval Array of uint32_t with the sizes of the matrix. End of Array is represented with -1
 */
uint32_t* get_shape(char* dictstring);

/**
 * \brief Get the dimensions of the array or matrix
 *
 * \note The user is responsible of freeing the uint32_t*
 *
 * \param[in] fp pass by reference a pointer to an opened FILE
 * \retval Array of uint32_t with the sizes of the matrix. Last value is -1
 */
uint32_t* get_shape_file(FILE* fp);

float64_t** allocate_matrix_f64(float64_t* data, int rows, int cols);

float64_t* get_1D_array_f64(FILE* fp);

float64_t** get_2D_matrix_f64(FILE* fp);

int32_t* get_1D_array_int32(FILE* fp);

int32_t** get_2D_matrix_int32(FILE* fp);

int get_2D_matrix_shape(FILE *fp, uint32_t *n_rows, uint32_t *n_cols);

int get_1D_array_len(FILE *fp, uint32_t *len);

/**
 * \brief Get the error number for a NULL returning function
 *
 * \retval NPY error number.
 */
int npyerrno();

#endif
