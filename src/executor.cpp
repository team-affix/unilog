#include <filesystem>
#include <fstream>
#include <iterator>
#include <functional>
#include <deque>
#include "executor.hpp"

namespace unilog
{
    bool execute(const axiom_statement &a_axiom_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // create term refs for each argument
        /////////////////////////////////////////
        term_t l_args = PL_new_term_refs(3);
        term_t l_module_path = l_args;
        term_t l_tag = l_args + 1;
        term_t l_theorem = l_args + 2;

        /////////////////////////////////////////
        // set up arguments
        /////////////////////////////////////////
        if (!(PL_unify(l_module_path, a_module_path) &&
              PL_unify(l_tag, a_axiom_statement.m_tag) &&
              PL_unify(l_theorem, a_axiom_statement.m_theorem)))
            return false;

        /////////////////////////////////////////
        // retrieve the associated predicate
        /////////////////////////////////////////
        predicate_t l_predicate = PL_predicate("decl_theorem", 3, NULL);

        if (!l_predicate)
            throw std::runtime_error("Failed to get decl_theorem prolog predicate.");

        /////////////////////////////////////////
        // open the query, supplying the args
        /////////////////////////////////////////
        qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, l_predicate, l_args);

        /////////////////////////////////////////
        // attempt to get a single match
        /////////////////////////////////////////
        if (!PL_next_solution(l_query))
            return false;

        /////////////////////////////////////////
        // close the prolog query
        /////////////////////////////////////////
        PL_close_query(l_query);

        PL_discard_foreign_frame(l_frame);

        return true;
    }

    bool execute(const guide_statement &a_guide_statement, term_t a_module_path)
    {
        return true;
    }

    bool execute(const infer_statement &a_infer_statement, term_t a_module_path)
    {
        return true;
    }

    bool execute(const refer_statement &a_refer_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // construct new term refs for arguments
        /////////////////////////////////////////
        term_t l_args = PL_new_term_refs(3);
        term_t l_list_0 = l_args;
        term_t l_list_1 = l_args + 1;
        term_t l_new_module_path = l_args + 2;

        /////////////////////////////////////////
        // set up arguments
        /////////////////////////////////////////

        // arg1
        if (!PL_unify(l_list_0, a_module_path))
            return false;

        // arg2
        if (!(PL_put_nil(l_list_1) &&
              PL_cons_list(l_list_1, a_refer_statement.m_tag, l_list_1)))
            return false;

        /////////////////////////////////////////
        // retrieve the append predicate
        /////////////////////////////////////////
        predicate_t l_predicate = PL_predicate("append", 3, NULL); // append/3

        if (!l_predicate)
            throw std::runtime_error("Error: failed to retrieve append/3 predicate.");

        /////////////////////////////////////////
        // open the query, supplying the args
        /////////////////////////////////////////
        qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, l_predicate, l_args);

        /////////////////////////////////////////
        // attempt to get a single match (executing the append function)
        /////////////////////////////////////////
        if (!PL_next_solution(l_query))
            return false;

        /////////////////////////////////////////
        // close the prolog query
        /////////////////////////////////////////
        PL_close_query(l_query);

        /////////////////////////////////////////
        // extract file path string from atom
        /////////////////////////////////////////
        char *l_file_path_c_str;
        if (!PL_get_atom_chars(a_refer_statement.m_file_path, &l_file_path_c_str))
            return false;

        /////////////////////////////////////////
        // construct fs path objects
        /////////////////////////////////////////

        namespace fs = std::filesystem;
        fs::path l_file_path = l_file_path_c_str;
        fs::path l_file_parent_path = l_file_path.parent_path();
        fs::path l_cwd = fs::current_path();

        /////////////////////////////////////////
        // open filestream
        /////////////////////////////////////////
        std::ifstream l_ifs(l_file_path);

        /////////////////////////////////////////
        // set cwd to parent path of the referee
        /////////////////////////////////////////
        fs::current_path(l_file_parent_path);

        /////////////////////////////////////////
        // execute all statements in file
        /////////////////////////////////////////
        std::istream_iterator<statement> l_it(l_ifs);
        std::istream_iterator<statement> l_end;

        for (; l_it != l_end; ++l_it)
        {
            std::visit(
                [l_new_module_path](const auto &a_statement)
                { execute(a_statement, l_new_module_path); }, *l_it);
        }

        /////////////////////////////////////////
        // reset cwd to the original cwd
        /////////////////////////////////////////
        fs::current_path(l_cwd);

        PL_discard_foreign_frame(l_frame);

        return true;
    }
}

#ifdef UNIT_TEST

#include "test_utils.hpp"

////////////////////////////////
//// HELPER FUNCTIONS
////////////////////////////////

// static void call_predicate_once(const std::string &a_predicate_name, int a_arity, )

static void retract_all(term_t a_clause_head)
{
    fid_t l_frame = PL_open_foreign_frame();

    /////////////////////////////////////////
    // constructs term refs for args
    /////////////////////////////////////////
    term_t l_args = PL_new_term_refs(1);
    term_t l_clause_head = l_args;

    /////////////////////////////////////////
    // set up arguments for retractall
    /////////////////////////////////////////
    if (!PL_unify(l_clause_head, a_clause_head))
        throw std::runtime_error("Error: failed to unify.");

    /////////////////////////////////////////
    // get predicate 'retractall'
    /////////////////////////////////////////
    predicate_t l_predicate = PL_predicate("retractall", 1, NULL);

    if (!l_predicate)
        throw std::runtime_error("Error: failed to retrieve retractall/1 predicate.");

    /////////////////////////////////////////
    // open the query, supplying the args
    /////////////////////////////////////////
    qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, l_predicate, l_args);

    /////////////////////////////////////////
    // attempt to get a single match (executing the retractall function)
    /////////////////////////////////////////
    if (!PL_next_solution(l_query))
        throw std::runtime_error("Error: failed to execute retractall/1 predicate.");

    /////////////////////////////////////////
    // close the prolog query
    /////////////////////////////////////////
    PL_close_query(l_query);

    PL_discard_foreign_frame(l_frame);
}

static void wipe_database()
{
    fid_t l_frame = PL_open_foreign_frame();

    /////////////////////////////////////////
    // creates the head of clause: theorem(_, _, _)
    /////////////////////////////////////////
    term_t l_theorem_clause_head = PL_new_term_ref();
    functor_t l_theorem_functor = PL_new_functor(PL_new_atom("theorem"), 3);
    PL_cons_functor(
        l_theorem_clause_head, l_theorem_functor,
        PL_new_term_ref(), PL_new_term_ref(), PL_new_term_ref());

    /////////////////////////////////////////
    // creates the head of clause: guide(_, _, _)
    /////////////////////////////////////////
    term_t l_guide_clause_head = PL_new_term_ref();
    functor_t l_guide_functor = PL_new_functor(PL_new_atom("theorem"), 3);
    PL_cons_functor(
        l_guide_clause_head, l_guide_functor,
        PL_new_term_ref(), PL_new_term_ref(), PL_new_term_ref());

    /////////////////////////////////////////
    // retract all dynamic statements
    /////////////////////////////////////////
    retract_all(l_theorem_clause_head);
    retract_all(l_guide_clause_head);

    PL_discard_foreign_frame(l_frame);
}

////////////////////////////////
////////////////////////////////

static void test_wipe_database()
{
}

static void test_execute_axiom_statement()
{
}

void test_executor_main()
{
    constexpr bool ENABLE_DEBUG_LOGS = true;

    TEST(test_wipe_database);
    TEST(test_execute_axiom_statement);
}

#endif
