#include <Python.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdint.h>
#include "numpy/arrayobject.h"
#include "libpq-fe.h"

// For ntohl, etc
#include <netinet/in.h>

using namespace std;

#define debug 0
#define debug2 0

/* Macro to convert to host byte-ordering, 8-byte int or float */
#define ntohll(x) ((((npy_uint64)ntohl(x)) << 32) + ntohl(x >> 32))

// NPY_BYTE_ORDER is new in 1.3.0, so implement here
#ifndef NPY_BYTE_ORDER
    #include <endian.h>
    #define NPY_BYTE_ORDER __BYTE_ORDER
    #if (__BYTE_ORDER == __LITTLE_ENDIAN)
        #define NPY_LITTLE_ENDIAN
    #elif (__BYTE_ORDER == __BIG_ENDIAN)
        #define NPY_BIG_ENDIAN
    #else
        #error Unknown machine endianness detected.
    #endif
#endif

// This is my thing
#ifdef NPY_LITTLE_ENDIAN
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
#else
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1
#endif


// Hold some info about the fields in a result structure.  
typedef struct {
    // The number of bytes in each field.
    vector<npy_intp> nbytes;
    // The number of elements with the above length in bytes
    vector<npy_intp> nel;
    // Is this an array?
    vector<int> isarray;
    // Is this a string?  We don't count char(1) here
    vector<int> isstring; 
} FIELD_INFO;

// the c in front is to distinguish from the inherited python class
class cPgNumpy {

    public:
        cPgNumpy();
        cPgNumpy(string conninfo);
        cPgNumpy(const char* conninfo);
        ~cPgNumpy();

        void open();
        void open(string conninfo);
        void open(const char* conninfo);


        // Execute a query
        void execute(string query_string);
        void execute(const char* query_string);

        // fetch results as an array
        PyObject* fetchall();


        // Write the results to a file or stdout. Requires running
        // use_text() to get results in text format
        long long write(const char* filename=NULL);

        // return results as text instead of binary.  Note, currently we don't
        // support fetching the results into numpy arrays for text retrevial,
        // but setting text is required for the write() method to write to a
        // file or stdout
        void use_text();

        // should we use array decorators for array writing? This will
        // use { } to delineate array dimensions.  This is required
        // for COPYing back into a table.  DEFAULT is true.
        void set_decorators(bool decorate);



        // Execute the query and use a cursor to retrieve the results in
        // chunks, writing to a file or by default stdout.  Unlike write(),
        // does *not* require use_text() to be set.
        //
        // This does not use the query execution infrastructure of cPgNumpy,
        // but rather is rolled specially for the cursor
        //
        // The return value is the number of rows written
        long long execwrite(
                const char* query,
                const char* filename=NULL);
        // size of fetch chunks when doing an execwrite
        // DEFAULT is 1000
        void set_fetch_count(long long fetchcount);


        long long ntuples() {
            return mNtuples;
        }
        long long nfields() {
            return mNfields;
        }
        int status() {
            return mResultStatus;
        }

        // explicitly set fields lengths.  This is useful for varchar types,
        // since for those types all the returned values must be scanned to get
        // the *largest*, and the output array field will have those
        // dimensions.  With this setting you can shortcut that.
        void set_field_lengths(PyObject* flenDict);
        void clear_field_lengths();

        void clear();
        void close();

    private:
        // private methods

        // run this after catching a SIGINT
        void _cancel_after_sigint(PyOS_sighandler_t old_inthandler);

        void _set_defaults();
        void _print_flush(const char *text);
        void _print_debug(const char *text);

        void _check_status();

        void _make_descr();
        void _fill_array(PyObject* ret);

        void _store_field(
                char* input,   // Input postgresql data 
                int flen,      // length of this field in the input data
                char* output,  // Output array data
                int nbytes,  // number of available bytes in the output array
                int isarray,   // is this an array?
                int isstring); // is this a string?

        // Fill a scalar with the appropriate number of bytes.  Swap bytes if 
        // necessary
        void _fill_scalar(
                char* input,     // Input postgresql data 
                int32_t flen,  // Lenght of this field in the input data 
                char* output,    // Output array data
                int nbytes,  // number of bytes available in the output data 
                int isstring);    // Is this a string?

        void _fill_array(
                char* input,    // Input postgresql data 
                char* output,   // Output array data
                int nbytes, // Number of bytes available in output data field
                int isstring);  // Is this a string?


        void _tohost(char* data, int nbytes);
        int32_t _tohost(int32_t num);

        void _pg_type_info(
                int pgtype, 
                string &nptype, 
                int& nbytes, 
                int& isarray);
        PyObject* _get_array_field_shape(
                int fieldnum, 
                npy_intp& totlen,
                int32_t& max_flen);
        int32_t _pg_string_nbytes(
                string& fname, 
                int fnum, 
                int ftype,
                int isarray, 
                int32_t max_flen);


        long int _long_from_dict(PyObject* dict, const char* name);

        // private data

        // If 1 run the next execute with results as binary.
        int mBinary;

        // was the last execute() run as ascii or binary?
        int mLastExecWasBinary;

        // Should we keep the {} decorators for arrays?
        bool mUseArrayDecorators;
        // fetch size for execwrite. Default is 1000
        long long mFetchCount;

        FIELD_INFO mFieldInfo;
        string mConnectInfo;

        PGconn *mConn;
        PGresult* mResult;

        int mResultStatus;

        // User input field length dictionary
        PyObject* mFlenDict;

        // Number of tuples, fields
        npy_intp mNtuples;
        npy_intp mNfields;

        // description list
        PyArray_Descr* mDescr;
};

