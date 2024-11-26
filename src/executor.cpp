#include <filesystem>
#include <fstream>
#include <iterator>
#include <functional>
#include <deque>
#include "executor.hpp"

#define CALL_PRED(name, arity, arg0) \
    (PL_call_predicate(NULL, PL_Q_NORMAL, PL_predicate(name, arity, NULL), arg0))

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
        // execute decl_theorem
        /////////////////////////////////////////
        if (!CALL_PRED("decl_theorem", 3, l_args))
            return false;

        PL_discard_foreign_frame(l_frame);

        return true;
    }

    bool execute(const guide_statement &a_guide_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // create term refs for each argument
        /////////////////////////////////////////
        term_t l_args = PL_new_term_refs(3);
        term_t l_module_path = l_args;
        term_t l_tag = l_args + 1;
        term_t l_guide = l_args + 2;

        /////////////////////////////////////////
        // set up arguments
        /////////////////////////////////////////
        if (!(PL_unify(l_module_path, a_module_path) &&
              PL_unify(l_tag, a_guide_statement.m_tag) &&
              PL_unify(l_guide, a_guide_statement.m_guide)))
            return false;

        /////////////////////////////////////////
        // execute decl_guide
        /////////////////////////////////////////
        if (!CALL_PRED("decl_guide", 3, l_args))
            return false;

        PL_discard_foreign_frame(l_frame);

        return true;
    }

    bool execute(const infer_statement &a_infer_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // create term refs for each argument
        /////////////////////////////////////////
        term_t l_args = PL_new_term_refs(3);
        term_t l_module_path = l_args;
        term_t l_tag = l_args + 1;
        term_t l_guide = l_args + 2;

        /////////////////////////////////////////
        // set up arguments
        /////////////////////////////////////////
        if (!(PL_unify(l_module_path, a_module_path) &&
              PL_unify(l_tag, a_infer_statement.m_tag) &&
              PL_unify(l_guide, a_infer_statement.m_guide)))
            return false;

        /////////////////////////////////////////
        // execute decl_guide
        /////////////////////////////////////////
        if (!CALL_PRED("infer", 3, l_args))
            return false;

        PL_discard_foreign_frame(l_frame);

        return true;
    }

    bool execute(const refer_statement &a_refer_statement, term_t a_module_path)
    {
        fid_t l_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // construct new module path
        /////////////////////////////////////////
        term_t l_new_module_path = PL_new_term_ref();
        if (!PL_cons_list(l_new_module_path, a_refer_statement.m_tag, a_module_path))
            return false;

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

        if (!l_ifs.good())
            return false;

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

static void wipe_database()
{
    /////////////////////////////////////////
    // creates the head of clause: theorem(_, _, _)
    /////////////////////////////////////////
    term_t l_theorem_clause_head = PL_new_term_ref();
    functor_t l_theorem_functor = PL_new_functor(PL_new_atom("theorem"), 3);
    if (!PL_cons_functor(
            l_theorem_clause_head, l_theorem_functor,
            PL_new_term_ref(), PL_new_term_ref(), PL_new_term_ref()))
        throw std::runtime_error("Error: failed to construct functor.");

    /////////////////////////////////////////
    // creates the head of clause: guide(_, _, _)
    /////////////////////////////////////////
    term_t l_guide_clause_head = PL_new_term_ref();
    functor_t l_guide_functor = PL_new_functor(PL_new_atom("guide"), 3);
    if (!PL_cons_functor(
            l_guide_clause_head, l_guide_functor,
            PL_new_term_ref(), PL_new_term_ref(), PL_new_term_ref()))
        throw std::runtime_error("Error: failed to construct functor.");

    /////////////////////////////////////////
    // retract all dynamic statements
    /////////////////////////////////////////
    // retract_all(l_theorem_clause_head);
    // retract_all(l_guide_clause_head);
    {
        fid_t l_query_frame = PL_open_foreign_frame();
        assert(CALL_PRED("retractall", 1, l_theorem_clause_head));
        PL_discard_foreign_frame(l_query_frame);
    };
    {
        fid_t l_query_frame = PL_open_foreign_frame();
        assert(CALL_PRED("retractall", 1, l_guide_clause_head));
        PL_discard_foreign_frame(l_query_frame);
    };
}

////////////////////////////////
////////////////////////////////

static void test_query_first_solution()
{
    fid_t l_frame = PL_open_foreign_frame();

    term_t l_argument = PL_new_term_ref();

    /////////////////////////////////////////
    // right now, argument is a variable thus atom/1 will fail.
    /////////////////////////////////////////
    assert(!CALL_PRED("atom", 1, l_argument));

    /////////////////////////////////////////
    // set argument to an atom using PL_unify
    /////////////////////////////////////////
    term_t l_atom_term = PL_new_term_ref();
    assert(PL_put_atom_chars(l_atom_term, "I am an atom"));
    assert(PL_unify(l_atom_term, l_argument));

    /////////////////////////////////////////
    // now, argument is an atom, thus atom/1 will succeed
    /////////////////////////////////////////
    assert(CALL_PRED("atom", 1, l_argument));

    PL_discard_foreign_frame(l_frame);
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
    assert(CALL_PRED("assertz", 1, l_clause_0));
    assert(CALL_PRED("assertz", 1, l_clause_1));
    assert(CALL_PRED("assertz", 1, l_clause_2));

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
    assert(CALL_PRED("retractall", 1, l_clause_0));
    assert(CALL_PRED("retractall", 1, l_clause_1));
    assert(CALL_PRED("retractall", 1, l_clause_2));

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
        functor_t l_guide_functor = PL_new_functor(PL_new_atom("guide"), 3);

        term_t l_theorem_clause = PL_new_term_ref();
        term_t l_guide_clause = PL_new_term_ref();
        assert(PL_cons_functor(l_theorem_clause, l_theorem_functor,
                               l_atom_0, l_atom_1, l_atom_2) &&
               PL_cons_functor(l_guide_clause, l_guide_functor,
                               l_atom_0, l_atom_1, l_atom_2));

        // /////////////////////////////////////////
        // // add clauses to db
        // /////////////////////////////////////////
        assert(CALL_PRED("assertz", 1, l_theorem_clause));
        assert(CALL_PRED("assertz", 1, l_guide_clause));

        /////////////////////////////////////////
        // ensure these statements unify (since clauses are bodyless, clause IS head)
        /////////////////////////////////////////
        assert(CALL_PRED("theorem", 3, l_args));
        assert(CALL_PRED("guide", 3, l_args));

        /////////////////////////////////////////
        // wipe the database
        /////////////////////////////////////////
        wipe_database();

        /////////////////////////////////////////
        // ensure these statements do NOT unify
        /////////////////////////////////////////
        assert(!CALL_PRED("theorem", 3, l_args));
        assert(!CALL_PRED("guide", 3, l_args));
    };

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_axiom_statement()
{
    using unilog::axiom_statement;
    using unilog::execute;

    fid_t l_frame = PL_open_foreign_frame();

    term_t l_args = PL_new_term_refs(3);
    term_t l_module_stack = l_args;
    term_t l_tag = l_args + 1;
    term_t l_theorem = l_args + 2;

    assert(
        PL_unify(l_module_stack,
                 make_list({
                     make_atom("daniel"),
                     make_atom("jake"),
                 })));
    assert(PL_unify(l_tag, make_atom("a0")));

    /////////////////////////////////////////
    // ensure we CANNOT find theorem with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(!CALL_PRED("theorem", 3, l_args));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // create the theorem s-expression which we will declare
    /////////////////////////////////////////
    term_t l_declared_theorem =
        make_list({
            make_atom("claims"),
            make_atom("leon"),
            make_atom("x"),
        });

    /////////////////////////////////////////
    // execute the axiom statement (in module path: l_module_stack)
    /////////////////////////////////////////
    assert(
        execute(
            axiom_statement{
                .m_tag = l_tag,
                .m_theorem = l_declared_theorem,
            },
            l_module_stack));

    /////////////////////////////////////////
    // ensure we CAN find theorem with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(CALL_PRED("theorem", 3, l_args));
        // assert(CALL_PRED("writeln", 1, l_module_stack));
        // assert(CALL_PRED("writeln", 1, l_tag));
        // assert(CALL_PRED("writeln", 1, l_theorem));
        assert(equal_forms(l_theorem, l_declared_theorem));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // ensure we do NOT have to worry about info persisting to next test case
    /////////////////////////////////////////
    wipe_database();

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_guide_statement()
{
    using unilog::execute;
    using unilog::guide_statement;

    fid_t l_frame = PL_open_foreign_frame();

    term_t l_args = PL_new_term_refs(3);
    term_t l_module_stack = l_args;
    term_t l_tag = l_args + 1;
    term_t l_guide = l_args + 2;

    assert(
        PL_unify(l_module_stack,
                 make_list({
                     make_atom("daniel"),
                     make_atom("jake"),
                 })));
    assert(PL_unify(l_tag, make_atom("g0")));

    /////////////////////////////////////////
    // ensure we CANNOT find guide with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(!CALL_PRED("guide", 3, l_args));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // create the guide s-expression which we will declare
    /////////////////////////////////////////
    term_t l_declared_guide =
        make_list({
            make_atom("mp"),
            make_list({
                make_atom("theorem"),
                make_atom("a0"),
            }),
            make_list({
                make_atom("theorem"),
                make_atom("a1"),
            }),
        });

    /////////////////////////////////////////
    // execute the guide statement (in module path: l_module_stack)
    /////////////////////////////////////////
    assert(
        execute(
            guide_statement{
                .m_tag = l_tag,
                .m_guide = l_declared_guide,
            },
            l_module_stack));

    /////////////////////////////////////////
    // ensure we CAN find guide with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(CALL_PRED("guide", 3, l_args));
        // assert(CALL_PRED("writeln", 1, l_module_stack));
        // assert(CALL_PRED("writeln", 1, l_tag));
        // assert(CALL_PRED("writeln", 1, l_guide));
        assert(equal_forms(l_guide, l_declared_guide));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // ensure we do NOT have to worry about info persisting to next test case
    /////////////////////////////////////////
    wipe_database();

    PL_discard_foreign_frame(l_frame);
}

static void test_execute_infer_statement()
{
    using unilog::axiom_statement;
    using unilog::execute;
    using unilog::guide_statement;
    using unilog::infer_statement;

    fid_t l_frame = PL_open_foreign_frame();

    term_t l_args = PL_new_term_refs(3);
    term_t l_module_stack = l_args;
    term_t l_tag = l_args + 1;
    term_t l_theorem = l_args + 2;

    assert(
        PL_unify(l_module_stack,
                 make_list({
                     make_atom("daniel"),
                     make_atom("jake"),
                 })));
    assert(PL_unify(l_tag, make_atom("i0")));

    /////////////////////////////////////////
    // ensure we CANNOT find theorem with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(!CALL_PRED("theorem", 3, l_args));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // execute the setup statements (in module path: l_module_stack)
    /////////////////////////////////////////
    assert(
        execute(
            guide_statement{
                .m_tag = make_atom("g0"),
                .m_guide = make_list({
                    make_atom("mp"),
                    make_list({
                        make_atom("theorem"),
                        make_atom("a0"),
                    }),
                    make_list({
                        make_atom("theorem"),
                        make_atom("a1"),
                    }),
                }),
            },
            l_module_stack));
    assert(
        execute(
            axiom_statement{
                .m_tag = make_atom("a0"),
                .m_theorem = make_list({
                    make_atom("if"),
                    make_atom("y"),
                    make_atom("x"),
                }),
            },
            l_module_stack));
    assert(
        execute(
            axiom_statement{
                .m_tag = make_atom("a1"),
                .m_theorem = make_atom("x"),
            },
            l_module_stack));

    /////////////////////////////////////////
    // execute the infer_statement
    /////////////////////////////////////////
    assert(
        execute(
            infer_statement{
                .m_tag = l_tag,
                .m_guide = make_list({
                    make_atom("guide"),
                    make_atom("g0"),
                }),
            },
            l_module_stack));

    /////////////////////////////////////////
    // ensure we CAN find theorem with this module stack + tag
    /////////////////////////////////////////
    {
        fid_t l_unification_frame = PL_open_foreign_frame();
        assert(CALL_PRED("theorem", 3, l_args));
        // assert(CALL_PRED("writeln", 1, l_module_stack));
        // assert(CALL_PRED("writeln", 1, l_tag));
        // assert(CALL_PRED("writeln", 1, l_theorem));
        assert(equal_forms(l_theorem, make_atom("y")));
        PL_discard_foreign_frame(l_unification_frame);
    };

    /////////////////////////////////////////
    // ensure we do NOT have to worry about info persisting to next test case
    /////////////////////////////////////////
    wipe_database();

    PL_discard_foreign_frame(l_frame);
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
        std::vector<guide> m_guides;
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
                .m_guides = std::vector<guide>({

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
                .m_guides = std::vector<guide>({
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
                .m_guides = std::vector<guide>({
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
                .m_guides = std::vector<guide>({

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
                .m_guides = std::vector<guide>({

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
                .m_guides = std::vector<guide>({

                }),
            },
        };

    for (const file_test_case &l_file_test_case : l_file_test_cases)
    {
        fid_t l_case_frame = PL_open_foreign_frame();

        /////////////////////////////////////////
        // execute the guide statement (in module path: l_module_stack)
        /////////////////////////////////////////
        assert(execute(l_file_test_case.m_refer_statement, l_file_test_case.m_module_stack));

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

            qid_t l_query = PL_open_query(NULL, PL_Q_NORMAL, PL_predicate("guide", 3, NULL), l_content_args);

            int i = 0;

            // loop thru extracting guides
            for (; PL_next_solution(l_query); ++i)
            {
                fid_t l_it_frame = PL_open_foreign_frame();

                assert((size_t)i < l_file_test_case.m_guides.size()); // make sure we do not go over expected #
                // assert(CALL_PRED("writeln", 1, l_content_module_stack));
                // assert(CALL_PRED("writeln", 1, l_content_tag));
                // assert(CALL_PRED("writeln", 1, l_content_sexpr));

                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_guides[i][0]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_guides[i][1]));
                // assert(CALL_PRED("writeln", 1, l_file_test_case.m_guides[i][2]));

                assert(equal_forms(l_content_module_stack, l_file_test_case.m_guides[i][0]));
                assert(equal_forms(l_content_tag, l_file_test_case.m_guides[i][1]));
                assert(equal_forms(l_content_sexpr, l_file_test_case.m_guides[i][2]));

                PL_discard_foreign_frame(l_it_frame);
            }

            // make sure we made it all the way thru the list
            assert((size_t)i == l_file_test_case.m_guides.size());

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

    TEST(test_query_first_solution);
    TEST(test_assertz_and_retract_all);
    TEST(test_wipe_database);
    TEST(test_execute_axiom_statement);
    TEST(test_execute_guide_statement);
    TEST(test_execute_infer_statement);
    TEST(test_execute_refer_statement);
}

#endif
