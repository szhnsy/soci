//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ORACLE_H_INCLUDED
#define SOCI_ORACLE_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_ORACLE_SOURCE
#   define SOCI_ORACLE_DECL __declspec(dllexport)
#  else
#   define SOCI_ORACLE_DECL __declspec(dllimport)
#  endif // SOCI_ORACLE_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_ORACLE_DECL isn't defined yet define it now
#ifndef SOCI_ORACLE_DECL
# define SOCI_ORACLE_DECL
#endif

#include <soci/soci-backend.h>
#include <oci.h> // OCI
#include <sstream>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4512 4511)
#endif


namespace soci
{

class SOCI_ORACLE_DECL oracle_soci_error : public soci_error
{
public:
    oracle_soci_error(std::string const & msg, int errNum = 0);

    virtual error_category get_error_category() const { return cat_; }

    int err_num_;
    error_category cat_;
};


struct oracle_statement_backend;
struct oracle_standard_into_type_backend : details::standard_into_type_backend
{
    oracle_standard_into_type_backend(oracle_statement_backend &st)
        : statement_(st), defnp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    virtual void define_by_pos(int &position,
        void *data, details::exchange_type type);

    void read_from_lob(OCILobLocator * lobp, std::string & value);
    
    virtual void pre_exec(int num);
    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, bool calledFromFetch,
        indicator *ind);

    virtual void clean_up();

    oracle_statement_backend &statement_;

    OCIDefine *defnp_;
    sb2 indOCIHolder_;
    void *data_;
    void *ociData_;
    char *buf_;        // generic buffer
    details::exchange_type type_;

    ub2 rCode_;
};

struct oracle_vector_into_type_backend : details::vector_into_type_backend
{
    oracle_vector_into_type_backend(oracle_statement_backend &st)
        : statement_(st), defnp_(NULL), indOCIHolders_(NULL),
        data_(NULL), buf_(NULL), user_ranges_(true) {}

    virtual void define_by_pos(int &position,
        void *data, details::exchange_type type)
    {
        user_ranges_ = false;
        define_by_pos(position, data, type, 0, &end_var_);
    }
    
    virtual void define_by_pos(
        int & position, void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end);

    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, indicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();
    std::size_t full_size();

    virtual void clean_up();

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the define_by_pos)
    void prepare_indicators(std::size_t size);

    oracle_statement_backend &statement_;

    OCIDefine *defnp_;
    sb2 *indOCIHolders_;
    std::vector<sb2> indOCIHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;
    bool user_ranges_;
    std::size_t colSize_;    // size of the string column (used for strings)
    std::vector<ub2> sizes_; // sizes of data fetched (used for strings)

    std::vector<ub2> rCodes_;
};

struct oracle_standard_use_type_backend : details::standard_use_type_backend
{
    oracle_standard_use_type_backend(oracle_statement_backend &st)
        : statement_(st), bindp_(NULL), indOCIHolder_(0),
          data_(NULL), buf_(NULL) {}

    virtual void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly);
    virtual void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly);

    // common part for bind_by_pos and bind_by_name
    void prepare_for_bind(void *&data, sb4 &size, ub2 &oracleType, bool readOnly);

    // common helper for pre_use for LOB-directed wrapped types
    void write_to_lob(OCILobLocator * lobp, const std::string & value);
    
    // common lazy initialization of the temporary LOB object
    void lazy_temp_lob_init();
    
    virtual void pre_exec(int num);
    virtual void pre_use(indicator const *ind);
    virtual void post_use(bool gotData, indicator *ind);

    virtual void clean_up();

    oracle_statement_backend &statement_;

    OCIBind *bindp_;
    sb2 indOCIHolder_;
    void *data_;
    void *ociData_;
    bool readOnly_;
    char *buf_;        // generic buffer
    details::exchange_type type_;
};

struct oracle_vector_use_type_backend : details::vector_use_type_backend
{
    oracle_vector_use_type_backend(oracle_statement_backend &st)
        : statement_(st), bindp_(NULL), indOCIHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    virtual void bind_by_pos(int & position,
        void * data, details::exchange_type type)
    {
        bind_by_pos(position, data, type, 0, &end_var_);
    }
    
    virtual void bind_by_pos(int & position,
        void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end);
    
    virtual void bind_by_name(const std::string & name,
        void * data, details::exchange_type type)
    {
        bind_by_name(name, data, type, 0, &end_var_);
    }

    virtual void bind_by_name(std::string const &name,
        void *data, details::exchange_type type,
        std::size_t begin, std::size_t * end);

    // common part for bind_by_pos and bind_by_name
    void prepare_for_bind(void *&data, sb4 &size, ub2 &oracleType);

    // helper function for preparing indicators and sizes_ vectors
    // (as part of the bind_by_pos and bind_by_name)
    void prepare_indicators(std::size_t size);

    virtual void pre_use(indicator const *ind);

    virtual std::size_t size(); // active size (might be lower than full vector size)
    std::size_t full_size();    // actual size of the user-provided vector

    virtual void clean_up();

    oracle_statement_backend &statement_;

    OCIBind *bindp_;
    std::vector<sb2> indOCIHolderVec_;
    sb2 *indOCIHolders_;
    void *data_;
    char *buf_;        // generic buffer
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;

    // used for strings only
    std::vector<ub2> sizes_;
    std::size_t maxSize_;
};

struct oracle_session_backend;
struct oracle_statement_backend : details::statement_backend
{
    oracle_statement_backend(oracle_session_backend &session);

    virtual void alloc();
    virtual void clean_up();
    virtual void prepare(std::string const &query,
        details::statement_type eType);

    virtual exec_fetch_result execute(int number);
    virtual exec_fetch_result fetch(int number);

    virtual long long get_affected_rows();
    virtual int get_number_of_rows();
    virtual std::string get_parameter_name(int index) const;

    virtual std::string rewrite_for_procedure_call(std::string const &query);

    virtual int prepare_for_describe();
    virtual void describe_column(int colNum, data_type &dtype,
        std::string &columnName);

    // helper for defining into vector<string>
    std::size_t column_size(int position);

    virtual oracle_standard_into_type_backend * make_into_type_backend();
    virtual oracle_standard_use_type_backend * make_use_type_backend();
    virtual oracle_vector_into_type_backend * make_vector_into_type_backend();
    virtual oracle_vector_use_type_backend * make_vector_use_type_backend();

    oracle_session_backend &session_;

    OCIStmt *stmtp_;

    bool boundByName_;
    bool boundByPos_;
    bool noData_;
};

struct oracle_rowid_backend : details::rowid_backend
{
    oracle_rowid_backend(oracle_session_backend &session);

    ~oracle_rowid_backend();

    OCIRowid *rowidp_;
};

struct oracle_blob_backend : details::blob_backend
{
    oracle_blob_backend(oracle_session_backend &session);

    ~oracle_blob_backend();

    virtual std::size_t get_len();
    
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    
    virtual std::size_t read_from_start(char * buf, std::size_t toRead,
        std::size_t offset)
    {
        return read(offset + 1, buf, toRead);
    }
    
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    
    virtual std::size_t write_from_start(const char * buf, std::size_t toWrite,
        std::size_t offset)
    {
        return write(offset + 1, buf, toWrite);
    }
    
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    
    virtual void trim(std::size_t newLen);

    oracle_session_backend &session_;

    OCILobLocator *lobp_;
};

struct oracle_session_backend : details::session_backend
{
    oracle_session_backend(std::string const & serviceName,
        std::string const & userName,
        std::string const & password,
        int mode,
        bool decimals_as_strings = false,
        int charset = 0,
        int ncharset = 0);

    ~oracle_session_backend();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    virtual std::string get_table_names_query() const
    {
        return "select table_name"
            " from user_tables";
    }

    virtual std::string get_column_descriptions_query() const
    {
        return "select column_name,"
            " data_type,"
            " char_length as character_maximum_length,"
            " data_precision as numeric_precision,"
            " data_scale as numeric_scale,"
            " decode(nullable, 'Y', 'YES', 'N', 'NO') as is_nullable"
            " from user_tab_columns"
            " where table_name = :t";
    }
    
    virtual std::string create_column_type(data_type dt,
        int precision, int scale)
    {
        //  Oracle-specific SQL syntax:
        
        std::string res;
        switch (dt)
        {
        case dt_string:
            {
                std::ostringstream oss;
                
                if (precision == 0)
                {
                    oss << "clob";
                }
                else
                {
                    oss << "varchar(" << precision << ")";
                }
                
                res += oss.str();
            }
            break;
            
        case dt_date:
            res += "timestamp";
            break;

        case dt_double:
            {
                std::ostringstream oss;
                if (precision == 0)
                {
                    oss << "number";
                }
                else
                {
                    oss << "number(" << precision << ", " << scale << ")";
                }
                
                res += oss.str();
            }
            break;

        case dt_integer:
            res += "integer";
            break;

        case dt_long_long:
            res += "number";
            break;
            
        case dt_unsigned_long_long:
            res += "number";
            break;

        case dt_blob:
            res += "blob";
            break;

        case dt_xml:
            res += "xmltype";
            break;

        default:
            throw soci_error("this data_type is not supported in create_column");
        }

        return res;
    }
    virtual std::string add_column(const std::string & tableName,
        const std::string & columnName, data_type dt,
        int precision, int scale)
    {
        return "alter table " + tableName + " add " +
            columnName + " " + create_column_type(dt, precision, scale);
    }
    virtual std::string alter_column(const std::string & tableName,
        const std::string & columnName, data_type dt,
        int precision, int scale)
    {
        return "alter table " + tableName + " modify " +
            columnName + " " + create_column_type(dt, precision, scale);
    }
    virtual std::string empty_blob()
    {
        return "empty_blob()";
    }
    virtual std::string nvl()
    {
        return "nvl";
    }

    virtual std::string get_dummy_from_table() const { return "dual"; }

    virtual std::string get_backend_name() const { return "oracle"; }

    void clean_up();

    virtual oracle_statement_backend * make_statement_backend();
    virtual oracle_rowid_backend * make_rowid_backend();
    virtual oracle_blob_backend * make_blob_backend();

    bool get_option_decimals_as_strings() { return decimals_as_strings_; }

    // Return either SQLT_FLT or SQLT_BDOUBLE as the type to use when binding
    // values of C type "double" (the latter is preferable but might not be
    // always available).
    ub2 get_double_sql_type() const;

    OCIEnv *envhp_;
    OCIServer *srvhp_;
    OCIError *errhp_;
    OCISvcCtx *svchp_;
    OCISession *usrhp_;
    bool decimals_as_strings_;
};

struct oracle_backend_factory : backend_factory
{
	  oracle_backend_factory() {}
    virtual oracle_session_backend * make_session(
        connection_parameters const & parameters) const;
};

extern SOCI_ORACLE_DECL oracle_backend_factory const oracle;

extern "C"
{

// for dynamic backend loading
SOCI_ORACLE_DECL backend_factory const * factory_oracle();
SOCI_ORACLE_DECL void register_factory_oracle();

} // extern "C"

} // namespace soci

#endif
