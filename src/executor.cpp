#include <filesystem>
#include <fstream>
#include <iterator>
#include <functional>
#include <deque>

#include "executor.hpp"
#include "err_msg.hpp"

// custom row+col tracking streambuf for printing
//     call stack of files on exception throw
class charpos_streambuf : public std::streambuf
{
private:
    std::streambuf *m_underlying; // The original streambuf
    int m_row = 1;                // Start row from 1
    int m_col = 1;                // Start column from 0

protected:
    // Override underflow to handle character reading
    int_type underflow() override
    {
        return m_underlying->sgetc();
    }

    // Override uflow to handle consuming a character
    int_type uflow() override
    {
        int_type l_char = m_underlying->sbumpc(); // Consume the current character

        if (l_char == traits_type::eof())
            return traits_type::eof(); // Pass EOF back

        // increment column
        ++m_col;

        // check for incrementing row
        if (l_char == '\n')
        {
            ++m_row;
            m_col = 1;
        }

        return l_char;
    }

    // Explicitly disallow seeking
    std::streampos seekoff(std::streamoff, std::ios_base::seekdir, std::ios_base::openmode) override
    {
        return std::streampos(-1); // Indicate failure
    }

    std::streampos seekpos(std::streampos, std::ios_base::openmode) override
    {
        return std::streampos(-1); // Indicate failure
    }

public:
    charpos_streambuf(std::streambuf *buf) : m_underlying(buf) {}

    // Getters for row and column
    int row() const { return m_row; }
    int col() const { return m_col; }
};

static int call_predicate(const std::string &a_functor, const std::vector<term_t> &a_args)
{
    /////////////////////////////////////////
    // define predicate we wish to call
    /////////////////////////////////////////
    predicate_t l_predicate = PL_predicate(a_functor.c_str(), a_args.size(), NULL);

    /////////////////////////////////////////
    // construct contiguous term refs for args (must always declare at least 1)
    /////////////////////////////////////////
    term_t l_contiguous_args = PL_new_term_refs(std::max(1, (int)a_args.size()));

    /////////////////////////////////////////
    // unify supplied args with contiguous refs
    /////////////////////////////////////////
    for (int i = 0; i < (int)a_args.size(); ++i)
    {
        if (!PL_unify(l_contiguous_args + i, a_args[i]))
            throw std::runtime_error(ERR_MSG_UNIFY);
    }

    /////////////////////////////////////////
    // call the predicate finally, and return the result.
    /////////////////////////////////////////
    return PL_call_predicate(NULL, PL_Q_NORMAL, l_predicate, l_contiguous_args);
}

namespace unilog
{
    void execute(const axiom_statement &a_axiom_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // execute decl_theorem
        /////////////////////////////////////////
        if (!call_predicate("decl_theorem", {a_module_path, a_axiom_statement.m_tag, a_axiom_statement.m_theorem}))
            throw std::runtime_error(ERR_MSG_DECL_THEOREM);

        PL_discard_foreign_frame(l_frame);
    }

    void execute(const redir_statement &a_redir_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // execute decl_redir
        /////////////////////////////////////////
        if (!call_predicate("decl_redir", {a_module_path, a_redir_statement.m_tag, a_redir_statement.m_guide}))
            throw std::runtime_error(ERR_MSG_DECL_REDIR);

        PL_discard_foreign_frame(l_frame);
    }

    void execute(const infer_statement &a_infer_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        term_t l_theorem = PL_new_term_ref();

        /////////////////////////////////////////
        // first, query to get the theorem produced by the guide
        /////////////////////////////////////////
        if (!call_predicate("query", {a_module_path, a_infer_statement.m_guide, l_theorem}))
            throw std::runtime_error(ERR_MSG_INFER);

        /////////////////////////////////////////
        // declare the theorem with the provided tag
        /////////////////////////////////////////
        if (!call_predicate("decl_theorem", {a_module_path, a_infer_statement.m_tag, l_theorem}))
            throw std::runtime_error(ERR_MSG_DECL_THEOREM);

        PL_discard_foreign_frame(l_frame);
    }

    void execute(const refer_statement &a_refer_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // construct new module path
        /////////////////////////////////////////
        term_t l_new_module_path = PL_new_term_ref();
        if (!PL_cons_list(l_new_module_path, a_refer_statement.m_tag, a_module_path))
            throw std::runtime_error(ERR_MSG_CONS_LIST);

        /////////////////////////////////////////
        // extract file path string from atom
        /////////////////////////////////////////
        char *l_file_path_c_str;
        if (!PL_get_atom_chars(a_refer_statement.m_file_path, &l_file_path_c_str))
            throw std::runtime_error(ERR_MSG_GET_ATOM_CHARS);

        /////////////////////////////////////////
        // construct fs path objects
        /////////////////////////////////////////
        namespace fs = std::filesystem;
        fs::path l_canonical_file_path = fs::canonical(l_file_path_c_str);
        fs::path l_file_parent_path = l_canonical_file_path.parent_path();
        fs::path l_cwd = fs::current_path();

        /////////////////////////////////////////
        // ensure file_path is to a file
        /////////////////////////////////////////
        if (fs::is_directory(l_canonical_file_path))
            throw std::runtime_error(ERR_MSG_NOT_A_FILE);

        /////////////////////////////////////////
        // open filestream
        /////////////////////////////////////////
        std::ifstream l_ifs(l_canonical_file_path);

        if (!l_ifs.good())
            throw std::runtime_error(std::string(ERR_MSG_FILE_OPEN) + ": " + l_file_path_c_str);

        /////////////////////////////////////////
        // set cwd to parent path of the referee
        /////////////////////////////////////////
        fs::current_path(l_file_parent_path);

        /////////////////////////////////////////
        // construct a charpos tracking streambuf
        /////////////////////////////////////////
        charpos_streambuf l_cpos_sbuf(l_ifs.rdbuf());
        std::istream l_cpos_ifs(&l_cpos_sbuf);

        /////////////////////////////////////////
        // execute all statements in file
        /////////////////////////////////////////
        std::istream_iterator<statement> l_it(l_cpos_ifs);
        std::istream_iterator<statement> l_end;

        try
        {
            for (; l_it != l_end; ++l_it)
            {
                std::visit(
                    [l_new_module_path](const auto &a_statement)
                    { execute(a_statement, l_new_module_path); }, *l_it);
            }
        }
        catch (const std::runtime_error &l_err)
        {
            // unwinding exception (call stack)
            std::string l_unwind_msg =
                std::string(l_err.what()) +
                "\nin: " + l_canonical_file_path.string() +
                std::string(":") + std::to_string(l_cpos_sbuf.row()) +
                std::string(":") + std::to_string(l_cpos_sbuf.col());
            throw std::runtime_error(l_unwind_msg);
        }

        /////////////////////////////////////////
        // reset cwd to the original cwd
        /////////////////////////////////////////
        fs::current_path(l_cwd);

        PL_discard_foreign_frame(l_frame);
    }
}

void wipe_database()
{
    call_predicate("wipe_database", {});
}

#ifdef UNIT_TEST

#include "test_utils.hpp"

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

////////////////////////////////
////////////////////////////////

/////////////////////////////////////////
// helper struct(s)
/////////////////////////////////////////
struct stmt_decl
{
    term_t m_module_stack;
    unilog::statement m_statement;
};

static void test_call_predicate()
{
    fid_t l_frame = PL_open_foreign_frame();

    term_t l_x = PL_new_term_ref();
    term_t l_y = PL_new_term_ref();

    /////////////////////////////////////////
    // right now, both args variable so ==/2 fails
    /////////////////////////////////////////
    assert(!call_predicate("==", {l_x, l_y}));

    /////////////////////////////////////////
    // set argument to an atom using PL_unify
    /////////////////////////////////////////
    term_t l_atom_term = PL_new_term_ref();
    assert(PL_put_atom_chars(l_atom_term, "I am an atom"));
    assert(PL_unify(l_atom_term, l_x));

    /////////////////////////////////////////
    // right now, args not strictly equal
    /////////////////////////////////////////
    assert(!call_predicate("==", {l_x, l_y}));

    /////////////////////////////////////////
    // set argument to an atom using PL_unify
    /////////////////////////////////////////
    assert(PL_unify(l_atom_term, l_y));

    /////////////////////////////////////////
    // now, args are strictly equal
    /////////////////////////////////////////
    assert(call_predicate("==", {l_x, l_y}));

    PL_discard_foreign_frame(l_frame);
}

static void test_charpos_streambuf()
{
    using pos = std::pair<int, int>;

    data_points<std::string, pos> l_data_points =
        {
            {"abc", {1, 4}},
            {"abc\n", {2, 1}},
            {"ab\nc", {2, 2}},
            {"a\nbc", {2, 3}},
            {"fasdfjklf fgd dfFFFjf 7 8^&>?/.?\\ \r ", {1, 37}},
            {"fasdfjklf fgd dfFFFjf 7 8^&>?/.?\\ \r \n", {2, 1}},
            {"fasdfjklf fgd\n dfFFFjf 7 8^&>?/.?\\ \r ", {2, 24}},
        };

    for (const auto &[l_str, l_position] : l_data_points)
    {
        std::stringstream l_ss(l_str);
        charpos_streambuf l_sbuf(l_ss.rdbuf());
        std::istream l_charpos_istream(&l_sbuf);

        char l_char;
        while (l_charpos_istream.get(l_char))
            ; // extract all chars

        assert(l_position == (pos{l_sbuf.row(), l_sbuf.col()}));
    }

    {
        std::stringstream l_ss("a\n");
        charpos_streambuf l_sbuf(l_ss.rdbuf());
        std::istream l_charpos_istream(&l_sbuf);
        assert(l_charpos_istream.peek() == 'a');
        assert(l_sbuf.row() == 1);
        assert(l_sbuf.col() == 1);
        assert(l_charpos_istream.get() == 'a');
        assert(l_sbuf.row() == 1);
        assert(l_sbuf.col() == 2);
        assert(l_charpos_istream.peek() == '\n');
        assert(l_sbuf.row() == 1);
        assert(l_sbuf.col() == 2);
        assert(l_charpos_istream.get() == '\n');
        assert(l_sbuf.row() == 2);
        assert(l_sbuf.col() == 1);
    }

    {
        std::stringstream l_ss("\na");
        charpos_streambuf l_sbuf(l_ss.rdbuf());
        std::istream l_charpos_istream(&l_sbuf);
        assert(l_charpos_istream.peek() == '\n');
        assert(l_sbuf.row() == 1);
        assert(l_sbuf.col() == 1);
        assert(l_charpos_istream.get() == '\n');
        assert(l_sbuf.row() == 2);
        assert(l_sbuf.col() == 1);
        assert(l_charpos_istream.peek() == 'a');
        assert(l_sbuf.row() == 2);
        assert(l_sbuf.col() == 1);
        assert(l_charpos_istream.get() == 'a');
        assert(l_sbuf.row() == 2);
        assert(l_sbuf.col() == 2);
    }
}

static void test_assertz_and_retract_all()
{
    fid_t l_frame = PL_open_foreign_frame();

    functor_t l_functor_0 = PL_new_functor(PL_new_atom("pred0"), 1);
    functor_t l_functor_1 = PL_new_functor(PL_new_atom("pred1"), 1);
    functor_t l_functor_2 = PL_new_functor(PL_new_atom("pred2"), 1);

    term_t l_atom_0 = PL_new_term_ref();
    term_t l_atom_1 = PL_new_term_ref();
    term_t l_atom_2 = PL_new_term_ref();

    PL_put_atom_chars(l_atom_0, "abc0");
    PL_put_atom_chars(l_atom_1, "abc1");
    PL_put_atom_chars(l_atom_2, "abc2");

    term_t l_clause_0 = PL_new_term_ref();
    term_t l_clause_1 = PL_new_term_ref();
    term_t l_clause_2 = PL_new_term_ref();

    assert(PL_cons_functor(l_clause_0, l_functor_0, l_atom_0) &&
           PL_cons_functor(l_clause_1, l_functor_1, l_atom_1) &&
           PL_cons_functor(l_clause_2, l_functor_2, l_atom_2));

    // assertz(l_clause_0);
    // assertz(l_clause_1);
    // assertz(l_clause_2);
    assert(call_predicate("assertz", {l_clause_0}));
    assert(call_predicate("assertz", {l_clause_1}));
    assert(call_predicate("assertz", {l_clause_2}));

    predicate_t l_predicate_0 = PL_predicate("pred0", 1, NULL);
    predicate_t l_predicate_1 = PL_predicate("pred1", 1, NULL);
    predicate_t l_predicate_2 = PL_predicate("pred2", 1, NULL);

    /////////////////////////////////////////
    // make sure the predicates can be retrieved
    /////////////////////////////////////////
    assert(l_predicate_0 && l_predicate_1 && l_predicate_2);

    /////////////////////////////////////////
    // ensure these statements unify
    /////////////////////////////////////////
    {
        qid_t l_query_0 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_0, l_atom_0);
        assert(PL_next_solution(l_query_0));
        PL_close_query(l_query_0);

        qid_t l_query_1 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_1, l_atom_1);
        assert(PL_next_solution(l_query_1));
        PL_close_query(l_query_1);

        qid_t l_query_2 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_2, l_atom_2);
        assert(PL_next_solution(l_query_2));
        PL_close_query(l_query_2);
    };

    /////////////////////////////////////////
    // ensure these statements do NOT unify
    /////////////////////////////////////////
    {
        qid_t l_query_0 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_0, l_atom_1);
        assert(!PL_next_solution(l_query_0));
        PL_close_query(l_query_0);

        qid_t l_query_1 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_1, l_atom_2);
        assert(!PL_next_solution(l_query_1));
        PL_close_query(l_query_1);

        qid_t l_query_2 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_2, l_atom_0);
        assert(!PL_next_solution(l_query_2));
        PL_close_query(l_query_2);
    };

    /////////////////////////////////////////
    // try to erase the entries from the DB.
    /////////////////////////////////////////
    assert(call_predicate("retractall", {l_clause_0}));
    assert(call_predicate("retractall", {l_clause_1}));
    assert(call_predicate("retractall", {l_clause_2}));

    /////////////////////////////////////////
    // ensure these statements do NOT unify
    // (the ones that unified earlier)
    /////////////////////////////////////////
    {
        qid_t l_query_0 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_0, l_atom_0);
        assert(!PL_next_solution(l_query_0));
        PL_close_query(l_query_0);

        qid_t l_query_1 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_1, l_atom_1);
        assert(!PL_next_solution(l_query_1));
        PL_close_query(l_query_1);

        qid_t l_query_2 = PL_open_query(NULL, PL_Q_NORMAL, l_predicate_2, l_atom_2);
        assert(!PL_next_solution(l_query_2));
        PL_close_query(l_query_2);
    };

    PL_discard_foreign_frame(l_frame);
}

static void test_wipe_database()
{
    fid_t l_frame = PL_open_foreign_frame();

    {
        term_t l_args = PL_new_term_refs(3);
        term_t l_atom_0 = l_args;
        term_t l_atom_1 = l_args + 1;
        term_t l_atom_2 = l_args + 2;

        PL_put_atom_chars(l_atom_0, "abc0");
        PL_put_atom_chars(l_atom_1, "abc1");
        PL_put_atom_chars(l_atom_2, "abc2");

        functor_t l_theorem_functor = PL_new_functor(PL_new_atom("theorem"), 3);
        functor_t l_guide_functor = PL_new_functor(PL_new_atom("redir"), 3);

        term_t l_theorem_clause = PL_new_term_ref();
        term_t l_guide_clause = PL_new_term_ref();
        assert(PL_cons_functor(l_theorem_clause, l_theorem_functor,
                               l_atom_0, l_atom_1, l_atom_2) &&
               PL_cons_functor(l_guide_clause, l_guide_functor,
                               l_atom_0, l_atom_1, l_atom_2));

        // /////////////////////////////////////////
        // // add clauses to db
        // /////////////////////////////////////////
        assert(call_predicate("assertz", {l_theorem_clause}));
        assert(call_predicate("assertz", {l_guide_clause}));

        /////////////////////////////////////////
        // ensure these statements unify (since clauses are bodyless, clause IS head)
        /////////////////////////////////////////
        assert(call_predicate("theorem", {l_atom_0, l_atom_1, l_atom_2}));
        assert(call_predicate("redir", {l_atom_0, l_atom_1, l_atom_2}));

        /////////////////////////////////////////
        // wipe the database
        /////////////////////////////////////////
        wipe_database();

        /////////////////////////////////////////
        // ensure these statements do NOT unify
        /////////////////////////////////////////
        assert(!call_predicate("theorem", {l_atom_0, l_atom_1, l_atom_2}));
        assert(!call_predicate("redir", {l_atom_0, l_atom_1, l_atom_2}));
    };

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_axiom_statement()
{
    using unilog::axiom_statement;
    using unilog::execute;

    fid_t l_frame = PL_open_foreign_frame();

    /////////////////////////////////////////
    // helper struct
    /////////////////////////////////////////
    struct axiom_decl
    {
        term_t m_module_stack;
        axiom_statement m_axiom_statement;
    };

    std::vector<axiom_decl> l_test_cases =
        {
            {
                .m_module_stack = make_list({}),
                .m_axiom_statement = axiom_statement{
                    .m_tag = make_atom("a0"),
                    .m_theorem = make_atom("x"),
                },
            },
            {
                .m_module_stack = make_list({}),
                .m_axiom_statement = axiom_statement{
                    .m_tag = make_atom("tag"),
                    .m_theorem = make_list({
                        make_atom("if"),
                        make_atom("y"),
                        make_atom("x"),
                    }),
                },
            },
            {
                .m_module_stack = make_list({
                    make_atom("root"),
                }),
                .m_axiom_statement = axiom_statement{
                    .m_tag = make_atom("tag"),
                    .m_theorem = make_list({
                        make_atom("if"),
                        make_atom("y"),
                        make_atom("x"),
                    }),
                },
            },
            {
                .m_module_stack = make_list({
                    make_atom("main"),
                    make_atom("root"),
                }),
                .m_axiom_statement = axiom_statement{
                    .m_tag = make_atom("tag"),
                    .m_theorem = make_list({
                        make_atom("if"),
                        make_atom("y"),
                        make_atom("x"),
                    }),
                },
            },
        };

    for (const auto &l_case : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        term_t l_theorem_result = PL_new_term_ref();

        /////////////////////////////////////////
        // before executing the axiom statement, querying should fail
        /////////////////////////////////////////
        assert(!call_predicate("theorem", {l_case.m_module_stack, l_case.m_axiom_statement.m_tag, l_theorem_result}));

        /////////////////////////////////////////
        // execute axiom statement
        /////////////////////////////////////////
        execute(l_case.m_axiom_statement, l_case.m_module_stack);

        /////////////////////////////////////////
        // querying should succeed
        /////////////////////////////////////////
        assert(call_predicate("theorem", {l_case.m_module_stack, l_case.m_axiom_statement.m_tag, l_theorem_result}));

        /////////////////////////////////////////
        // ensure theorem transferred properly
        /////////////////////////////////////////
        assert(equal_forms(l_theorem_result, l_case.m_axiom_statement.m_theorem));

        /////////////////////////////////////////
        // make sure to wipe the db between test cases
        /////////////////////////////////////////
        wipe_database();

        PL_close_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_redir_statement()
{
    using unilog::execute;
    using unilog::redir_statement;

    fid_t l_frame = PL_open_foreign_frame();

    /////////////////////////////////////////
    // helper struct
    /////////////////////////////////////////
    struct redir_decl
    {
        term_t m_module_stack;
        redir_statement m_redir_statement;
    };

    std::vector<redir_decl> l_test_cases =
        {
            {
                .m_module_stack = make_list({}),
                .m_redir_statement = redir_statement{
                    .m_tag = make_atom("r0"),
                    .m_guide = make_atom("x"),
                },
            },
            {
                .m_module_stack = make_list({
                    make_atom("root"),
                }),
                .m_redir_statement = redir_statement{
                    .m_tag = make_atom("tag"),
                    .m_guide = make_atom("x"),
                },
            },
            {
                .m_module_stack = make_list({
                    make_atom("main"),
                    make_atom("root"),
                }),
                .m_redir_statement = redir_statement{
                    .m_tag = make_atom("tag"),
                    .m_guide = make_atom("x"),
                },
            },
            {
                .m_module_stack = make_list({
                    make_atom("main"),
                    make_atom("root"),
                }),
                .m_redir_statement = redir_statement{
                    .m_tag = make_atom("tag"),
                    .m_guide = make_list({
                        make_atom("mp"),
                        make_list({
                            make_atom("t"),
                            make_atom("a0"),
                        }),
                        make_list({
                            make_atom("t"),
                            make_atom("a1"),
                        }),
                    }),
                },
            },
        };

    for (const auto &l_case : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        term_t l_guide_result = PL_new_term_ref();

        /////////////////////////////////////////
        // before executing the statement, querying should fail
        /////////////////////////////////////////
        assert(!call_predicate("redir", {l_case.m_module_stack, l_case.m_redir_statement.m_tag, l_guide_result}));

        /////////////////////////////////////////
        // execute statement
        /////////////////////////////////////////
        execute(l_case.m_redir_statement, l_case.m_module_stack);

        /////////////////////////////////////////
        // querying should succeed
        /////////////////////////////////////////
        assert(call_predicate("redir", {l_case.m_module_stack, l_case.m_redir_statement.m_tag, l_guide_result}));

        /////////////////////////////////////////
        // ensure content transferred properly
        /////////////////////////////////////////
        assert(equal_forms(l_guide_result, l_case.m_redir_statement.m_guide));

        /////////////////////////////////////////
        // make sure to wipe the db between test cases
        /////////////////////////////////////////
        wipe_database();

        PL_close_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_infer_statement()
{
    using unilog::axiom_statement;
    using unilog::execute;
    using unilog::infer_statement;
    using unilog::redir_statement;
    using unilog::statement;

    fid_t l_frame = PL_open_foreign_frame();

    struct conclusion
    {
        term_t m_module_stack;
        term_t m_theorem;
    };

    term_t l_universal_conclusion_tag = make_atom("conclusion");

    /////////////////////////////////////////
    // expect success data points
    /////////////////////////////////////////
    data_points<std::list<stmt_decl>, conclusion> l_test_cases =
        {
            {
                // this case is a basic integration test for inferences
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag1"),
                            .m_theorem = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = infer_statement{
                            .m_tag = l_universal_conclusion_tag,
                            .m_guide = make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag0"),
                                }),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag1"),
                                }),
                            }),
                        },
                    },
                },
                conclusion{
                    .m_module_stack = make_list({}),
                    .m_theorem = make_atom("y"),
                },
            },
            {
                // this case tests if module_stack is received correctly
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_list({
                                make_atom("claim"),
                                make_atom("m1"),
                                make_list({
                                    make_atom("if"),
                                    make_atom("y"),
                                    make_atom("x"),
                                }),
                            }),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({
                            make_atom("m1"),
                        }),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag1"),
                            .m_theorem = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = infer_statement{
                            .m_tag = l_universal_conclusion_tag,
                            .m_guide = make_list({
                                make_atom("bout"),
                                make_atom("m1"),
                                make_list({
                                    make_atom("mp"),
                                    make_list({
                                        make_atom("bin"),
                                        make_atom("m1"),
                                        make_list({
                                            make_atom("t"),
                                            make_atom("tag0"),
                                        }),
                                    }),
                                    make_list({
                                        make_atom("dout"),
                                        make_atom("m1"),
                                        make_list({
                                            make_atom("t"),
                                            make_atom("tag1"),
                                        }),
                                    }),
                                }),
                            }),
                        },
                    },
                },
                conclusion{
                    .m_module_stack = make_list({}),
                    .m_theorem = make_list({
                        make_atom("claim"),
                        make_atom("m1"),
                        make_atom("y"),
                    }),
                },
            },
            {
                // this tests to ensure module_stack is received correctly
                {
                    stmt_decl{
                        .m_module_stack = make_list({
                            make_atom("module"),
                        }),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({
                            make_atom("module"),
                        }),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag1"),
                            .m_theorem = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({
                            make_atom("module"),
                        }),
                        .m_statement = infer_statement{
                            .m_tag = l_universal_conclusion_tag,
                            .m_guide = make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag0"),
                                }),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag1"),
                                }),
                            }),
                        },
                    },
                },
                conclusion{
                    .m_module_stack = make_list({
                        make_atom("module"),
                    }),
                    .m_theorem = make_atom("y"),
                },
            },
        };

    for (const auto &[l_statements, l_conclusion] : l_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        term_t l_produced_theorem = PL_new_term_ref();

        /////////////////////////////////////////
        // before executing the statements, querying should fail
        /////////////////////////////////////////
        assert(!call_predicate("theorem", {l_conclusion.m_module_stack, l_universal_conclusion_tag, l_produced_theorem}));

        /////////////////////////////////////////
        // execute statements
        /////////////////////////////////////////
        for (const stmt_decl &l_stmt_decl : l_statements)
        {
            std::visit(
                [&l_stmt_decl](const auto &l_stmt)
                { execute(l_stmt, l_stmt_decl.m_module_stack); }, l_stmt_decl.m_statement);
        }

        /////////////////////////////////////////
        // querying should succeed
        /////////////////////////////////////////
        assert(call_predicate("theorem", {l_conclusion.m_module_stack, l_universal_conclusion_tag, l_produced_theorem}));

        /////////////////////////////////////////
        // ensure content transferred properly
        /////////////////////////////////////////
        assert(equal_forms(l_produced_theorem, l_conclusion.m_theorem));

        /////////////////////////////////////////
        // make sure to wipe the db between test cases
        /////////////////////////////////////////
        wipe_database();

        PL_close_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_program_throws()
{
    using unilog::axiom_statement;
    using unilog::infer_statement;
    using unilog::redir_statement;

    /////////////////////////////////////////
    // expect throw data points
    /////////////////////////////////////////
    data_points<std::list<stmt_decl>, std::string> l_throw_cases =
        {
            {
                // ensure theorem tags must be unique
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"), // duplicate tag
                            .m_theorem = make_atom("y"),
                        },
                    },
                },
                ERR_MSG_DECL_THEOREM,
            },
            {
                // ensure redirect tags must be unique
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = redir_statement{
                            .m_tag = make_atom("tag0"),
                            .m_guide = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = redir_statement{
                            .m_tag = make_atom("tag0"), // duplicate tag
                            .m_guide = make_atom("y"),
                        },
                    },
                },
                ERR_MSG_DECL_REDIR,
            },
            {
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag1"),
                            .m_theorem = make_atom("y"), // wrong theorem for inference to succeed
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = infer_statement{
                            .m_tag = make_atom("i0"),
                            .m_guide = make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag0"),
                                }),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag1"),
                                }),
                            }),
                        },
                    },
                },
                ERR_MSG_INFER,
            },
            {
                {
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag0"),
                            .m_theorem = make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = axiom_statement{
                            .m_tag = make_atom("tag1"),
                            .m_theorem = make_atom("x"),
                        },
                    },
                    stmt_decl{
                        .m_module_stack = make_list({}),
                        .m_statement = infer_statement{
                            .m_tag = make_atom("tag0"), // tag is not unique
                            .m_guide = make_list({
                                make_atom("mp"),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag0"),
                                }),
                                make_list({
                                    make_atom("t"),
                                    make_atom("tag1"),
                                }),
                            }),
                        },
                    },
                },
                ERR_MSG_DECL_THEOREM,
            },
        };

    for (const auto &[l_statements, l_err_msg] : l_throw_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        try
        {
            /////////////////////////////////////////
            // execute statements
            /////////////////////////////////////////
            for (const stmt_decl &l_stmt_decl : l_statements)
            {
                std::visit(
                    [&l_stmt_decl](const auto &l_stmt)
                    { execute(l_stmt, l_stmt_decl.m_module_stack); }, l_stmt_decl.m_statement);
            }
        }
        catch (const std::runtime_error &l_err)
        {
            assert(l_err.what() == l_err_msg);
        }

        /////////////////////////////////////////
        // make sure to wipe the db between test cases
        /////////////////////////////////////////
        wipe_database();

        PL_close_foreign_frame(l_case_frame);
    }
}

static void test_execute_refer_statement()
{
    fid_t l_frame = PL_open_foreign_frame();

    constexpr bool ENABLE_DEBUG_LOGS = true;

    using unilog::axiom_statement;
    using unilog::execute;
    using unilog::refer_statement;

    using theorem = std::array<term_t, 3>;
    using guide = std::array<term_t, 3>;

    struct file_test_case
    {
        term_t m_module_stack;
        refer_statement m_refer_statement;
        std::vector<theorem> m_theorems;
        std::vector<guide> m_redirs;
    };

    std::map<std::string, term_t> l_var_decl_alist;

    std::vector<file_test_case> l_file_test_cases =
        {
            file_test_case{
                .m_module_stack = make_list({
                    make_atom("daniel"),
                    make_atom("jake"),
                }),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("math"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_0/test.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("math"),
                            make_atom("daniel"),
                            make_atom("jake"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("math"),
                            make_atom("daniel"),
                            make_atom("jake"),
                        }),
                        make_atom("a1"),
                        make_atom("x"),
                    },
                }),
                .m_redirs = std::vector<guide>({

                }),
            },
            file_test_case{
                .m_module_stack = make_list({
                    make_atom("root"),
                }),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_1/main.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("b"),
                            make_atom("a"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("math"),
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("c"),
                            make_atom("d"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("math"),
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a1"),
                        make_atom("d"),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a1"),
                        make_atom("a"),
                    },
                }),
                .m_redirs = std::vector<guide>({
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("g0"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                    {
                        make_list({
                            make_atom("math"),
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_list({
                            make_atom("g0"),
                            make_var("X", l_var_decl_alist),
                            make_var("Y", l_var_decl_alist),
                        }),
                        make_list({
                            make_atom("mp"),
                            make_var("X", l_var_decl_alist),
                            make_var("Y", l_var_decl_alist),
                        }),
                    },
                }),
            },
            file_test_case{
                .m_module_stack = make_list({}),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_2/main.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("r1"),
                            make_atom("main"),
                        }),
                        make_atom("a0"),
                        make_list({
                                      make_atom("a0"),
                                      make_var("X", l_var_decl_alist),
                                  },
                                  make_var("X", l_var_decl_alist)),
                    },
                    {
                        make_list({
                            make_atom("r1"),
                            make_atom("main"),
                        }),
                        make_atom("a1"),
                        make_list({
                                      make_atom("a1"),
                                      make_var("Y", l_var_decl_alist),
                                  },
                                  make_var("Y", l_var_decl_alist)),
                    },
                    {
                        make_list({
                            make_atom("r1"),
                            make_atom("main"),
                        }),
                        make_atom("a2"),
                        make_list({
                                      make_atom("a2"),
                                      make_var("Z", l_var_decl_alist),
                                      make_var("Z", l_var_decl_alist),
                                      make_var("X", l_var_decl_alist),
                                  },
                                  make_var("X", l_var_decl_alist)),
                    },
                    {
                        make_list({
                            make_atom("r2"),
                            make_atom("main"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("+"),
                            make_atom("0"),
                            make_atom("0"),
                            make_atom("0"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r2"),
                            make_atom("main"),
                        }),
                        make_atom("a1"),
                        make_list({
                            make_atom("+"),
                            make_atom("1"),
                            make_atom("0"),
                            make_atom("1"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r2"),
                            make_atom("main"),
                        }),
                        make_atom("a2"),
                        make_list({
                            make_atom("+"),
                            make_atom("0"),
                            make_atom("1"),
                            make_atom("1"),
                        }),
                    },
                }),
                .m_redirs = std::vector<guide>({
                    {
                        make_list({
                            make_atom("r1"),
                            make_atom("main"),
                        }),
                        make_atom("g0"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r1"),
                            make_atom("main"),
                        }),
                        make_atom("g1"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r2"),
                            make_atom("main"),
                        }),
                        make_atom("g0"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r2"),
                            make_atom("main"),
                        }),
                        make_atom("g1"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                }),
            },
            file_test_case{
                .m_module_stack = make_list({}),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_3/main/main.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("r"),
                            make_atom("main"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("a0"),
                            make_atom("a0"),
                            make_atom("a0"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("main"),
                        }),
                        make_atom("tag"),
                        make_list({
                            make_atom("test"),
                        }),
                    },
                }),
                .m_redirs = std::vector<guide>({

                }),
            },
            file_test_case{
                .m_module_stack = make_list({
                    make_atom("start"),
                }),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_4/main.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("start"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("start"),
                        }),
                        make_atom("a1"),
                        make_atom("x"),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("start"),
                        }),
                        make_atom("a2"),
                        make_atom("y"),
                    },
                }),
                .m_redirs = std::vector<guide>({

                }),
            },
            file_test_case{
                .m_module_stack = make_list({}),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_5/main.u"),
                },
                .m_theorems = std::vector<theorem>({
                    {
                        make_list({
                            make_atom("referee"),
                            make_atom("main"),
                        }),
                        make_atom("a0"),
                        make_atom("x"),
                    },
                    {
                        make_list({
                            make_atom("referee"),
                            make_atom("main"),
                        }),
                        make_atom("a1"),
                        make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("referee"),
                            make_atom("main"),
                        }),
                        make_atom("i0"),
                        make_atom("y"),
                    },
                }),
                .m_redirs = std::vector<guide>({

                }),
            },
            file_test_case{
                .m_module_stack = make_list({
                    make_atom("root"),
                }),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_6/main.u"),
                },
                .m_theorems = {
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a1"),
                        make_atom("x"),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("i0"),
                        make_atom("y"),
                    },
                },
                .m_redirs = {
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("r0"),
                        make_list({
                            make_atom("mp"),
                            make_list({
                                make_atom("t"),
                                make_atom("a0"),
                            }),
                            make_list({
                                make_atom("t"),
                                make_atom("a1"),
                            }),
                        }),
                    },
                },
            },
            file_test_case{
                .m_module_stack = make_list({}),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_7/main.u"),
                },
                .m_theorems = {
                    {
                        make_list({
                            make_atom("main"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("claim"),
                            make_atom("m1"),
                            make_list({
                                make_atom("if"),
                                make_atom("y"),
                                make_atom("x"),
                            }),
                        }),
                    },
                    {
                        make_list({
                            make_atom("main"),
                        }),
                        make_atom("a1"),
                        make_list({
                            make_atom("claim"),
                            make_atom("m1"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("main"),
                        }),
                        make_atom("i0"),
                        make_list({
                            make_atom("claim"),
                            make_atom("m1"),
                            make_atom("y"),
                        }),
                    },
                },
                .m_redirs = {

                },
            },
            file_test_case{
                .m_module_stack = make_list({
                    make_atom("root"),
                }),
                .m_refer_statement = refer_statement{
                    .m_tag = make_atom("main"),
                    .m_file_path = make_atom("./src/test_input_files/executor_example_8/main.u"),
                },
                .m_theorems = {
                    {
                        make_list({
                            make_atom("r"),
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a0"),
                        make_list({
                            make_atom("if"),
                            make_atom("y"),
                            make_atom("x"),
                        }),
                    },
                    {
                        make_list({
                            make_atom("r"),
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("a1"),
                        make_atom("x"),
                    },
                    {
                        make_list({
                            make_atom("main"),
                            make_atom("root"),
                        }),
                        make_atom("i0"),
                        make_list({
                            make_atom("claim"),
                            make_atom("r"),
                            make_atom("y"),
                        }),
                    },
                },
                .m_redirs = {

                },
            },
        };

    for (const file_test_case &l_file_test_case : l_file_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // execute the guide statement (in module path: l_module_stack)
        /////////////////////////////////////////
        execute(l_file_test_case.m_refer_statement, l_file_test_case.m_module_stack);

        /////////////////////////////////////////
        // check database for theorems
        /////////////////////////////////////////
        {
            /////////////////////////////////////////
            // create args for retrieving theorems
            /////////////////////////////////////////
            term_t l_content_args = PL_new_term_refs(3);
            term_t l_content_module_stack = l_content_args;
            term_t l_content_tag = l_content_args + 1;
            term_t l_content_sexpr = l_content_args + 2;

            qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, PL_predicate("theorem", 3, NULL), l_content_args);

            int i = 0;

            // loop thru extracting theorems
            for (; PL_next_solution(l_query); ++i)
            {
                fid_t l_it_frame = PL_open_foreign_frame();

                assert((size_t)i < l_file_test_case.m_theorems.size()); // make sure we do not go over expected #
                // assert(CALL_PRED("writeln", 1, l_content_module_stack));
                // assert(CALL_PRED("writeln", 1, l_content_tag));
                // assert(CALL_PRED("writeln", 1, l_content_sexpr));

                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_theorems[i][0]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_theorems[i][1]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_theorems[i][2]));

                assert(equal_forms(l_content_module_stack, l_file_test_case.m_theorems[i][0]));
                assert(equal_forms(l_content_tag, l_file_test_case.m_theorems[i][1]));
                assert(equal_forms(l_content_sexpr, l_file_test_case.m_theorems[i][2]));

                PL_discard_foreign_frame(l_it_frame);
            }

            // make sure we made it all the way thru the list
            assert((size_t)i == l_file_test_case.m_theorems.size());

            PL_cut_query(l_query);
        };

        /////////////////////////////////////////
        // check database for guides
        /////////////////////////////////////////
        {
            /////////////////////////////////////////
            // create args for retrieving guides
            /////////////////////////////////////////
            term_t l_content_args = PL_new_term_refs(3);
            term_t l_content_module_stack = l_content_args;
            term_t l_content_tag = l_content_args + 1;
            term_t l_content_sexpr = l_content_args + 2;

            qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, PL_predicate("redir", 3, NULL), l_content_args);

            int i = 0;

            // loop thru extracting guides
            for (; PL_next_solution(l_query); ++i)
            {
                fid_t l_it_frame = PL_open_foreign_frame();

                assert((size_t)i < l_file_test_case.m_redirs.size()); // make sure we do not go over expected #
                // assert(CALL_PRED("writeln", 1, l_content_module_stack));
                // assert(CALL_PRED("writeln", 1, l_content_tag));
                // assert(CALL_PRED("writeln", 1, l_content_sexpr));

                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_redirs[i][0]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_redirs[i][1]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_redirs[i][2]));

                assert(equal_forms(l_content_module_stack, l_file_test_case.m_redirs[i][0]));
                assert(equal_forms(l_content_tag, l_file_test_case.m_redirs[i][1]));
                assert(equal_forms(l_content_sexpr, l_file_test_case.m_redirs[i][2]));

                PL_discard_foreign_frame(l_it_frame);
            }

            // make sure we made it all the way thru the list
            assert((size_t)i == l_file_test_case.m_redirs.size());

            PL_cut_query(l_query);
        };

        /////////////////////////////////////////
        // ensure we do NOT have to worry about info persisting to next test case
        /////////////////////////////////////////
        wipe_database();

        char *l_file_path_str;
        assert(PL_get_atom_chars(l_file_test_case.m_refer_statement.m_file_path, &l_file_path_str));

        LOG("success, referred file: " << l_file_path_str << std::endl);

        PL_discard_foreign_frame(l_case_frame);
    }

    PL_discard_foreign_frame(l_frame);
}

void test_executor_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_call_predicate);
    TEST(test_charpos_streambuf);
    TEST(test_assertz_and_retract_all);
    TEST(test_wipe_database);
    TEST(test_execute_axiom_statement);
    TEST(test_execute_redir_statement);
    TEST(test_execute_infer_statement);
    TEST(test_execute_program_throws);
    TEST(test_execute_refer_statement);
}

#endif
