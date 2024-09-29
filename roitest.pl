%%%%%%%%%%%%%%%%%%%%%%%
% DECLARATIONS (axioms and guides)
%%%%%%%%%%%%%%%%%%%%%%%

%    axiom([], a0, thm0),
%    axiom([], a1, [thm0, thm1]),
%    axiom([], a2, [_, abc, []]),
%    axiom([], a3, []),
%    axiom([], a4, 'quoted atom'),
%
%    axiom([], mp_imp_0, [if, consequent, antecedent]),
%    axiom([], mp_jus_0, antecedent).

%unilog(guide([], mp_imp_1), fact(theorem([if, consequent1, [and, jus0, jus1]]))).
%unilog(guide([], mp_jus_1), fact(theorem())).
%
%unilog(guide([], mp_imp_2), fact(theorem([if, c, [if, consequent, [and, antecedent]]]))).
%
%unilog(guide([], a5), fact(theorem([scope, m1, [if, b, a]]))).
%unilog(guide([], a6), fact(theorem([scope, m1, a]))).
%
%unilog(guide([], a7), fact(theorem([scope, m1, [scope, m2, [if, d, c]]]))).
%unilog(guide([], a8), fact(theorem([scope, m1, [scope, m2, c]]))).
%
%unilog(guide([m1], a0), fact(theorem(a))).
%unilog(guide([m1], a1), fact(theorem([if, b, a]))).
%unilog(guide([m1], a2), fact(theorem([scope, m2, [if, d, c]]))).
%
%unilog(guide([m1, m2], a0), fact(theorem(c))).
%unilog(guide([m1, m2], a1), fact(theorem([if, d, c]))).
%
%unilog(guide([], g0), fact(guide([gor, a0, a1, a2, a3, a4]))).
%
%unilog(guide([], imp_m1_consequent), fact(theorem(
%    [if,
%        [scope, m1, b],
%        a
%    ]
%))).
%
%unilog(guide([], jus_m1_consequent), fact(theorem(a))).
%
%unilog(guide([], imp_m2_consequent), fact(theorem(
%    [scope, m1,
%        [if,
%            c,
%            b
%        ]
%    ]
%))).
%
%% 'last' function definition, in root namespace
%unilog(guide([], root_last_bc), fact(theorem([last, [X], X]))).
%unilog(guide([], root_last_gc), fact(theorem(
%    [if,
%        [last, L, Last],
%        [and,
%            [cons, L, _, Rest],
%            [last, Rest, Last]
%        ]
%    ]
%))).
%unilog(guide([], root_last), fact(guide(
%    [gor,
%        root_last_bc,
%        [mp, root_last_gc, [conj, cons, root_last]]
%    ]
%))).
%
%% 'last' function definition, in alg, declared in root namespace
%unilog(guide([], root_alg_last_bc), fact(theorem([scope, alg, [last, [X], X]]))).
%unilog(guide([], root_alg_last_gc), fact(theorem(
%    [scope, alg,
%        [if,
%            [last, L, Last],
%            [and,
%                [cons, L, _, Rest],
%                [last, Rest, Last]
%            ]
%        ]
%    ]
%))).
%unilog(guide([], root_alg_last), fact(guide(
%    [gor,
%        root_alg_last_bc,
%        [mp, root_alg_last_gc, [conj, cons, root_alg_last]]
%    ]
%))).
%
%% 'last' function definition, in alg, declared in alg namespace
%unilog(guide([alg], alg_last_bc), fact(theorem([last, [X], X]))).
%unilog(guide([alg], alg_last_gc), fact(theorem(
%    [if,
%        [last, L, Last],
%        [and,
%            [cons, L, _, Rest],
%            [last, Rest, Last]
%        ]
%    ]
%))).
%unilog(guide([alg], alg_last), fact(guide(
%    [gor,
%        alg_last_bc,
%        [mp, alg_last_gc, [conj, cons, alg_last]]
%    ]
%))).
%
%% 'all' function definition, in alg, declared in alg namespace
%unilog(guide([alg], alg_all_bc), fact(theorem([all, [], _]))).
%unilog(guide([alg], alg_all_gc), fact(theorem(
%    [if,
%        [all, L, U],
%        [and,
%            [cons, L, X, Rest],
%            [U, X],
%            [all, Rest, U]
%        ]
%    ]
%))).
%unilog(guide([alg], [alg_all, UG]), fact(guide(
%    [gor,
%        alg_all_bc,
%        [mp, alg_all_gc, [conj, cons, UG, [alg_all, UG]]]
%    ]
%))).
%
%unilog(guide([], starts_with_c), fact(theorem(
%    [if,
%        [swc, S],
%        [and,
%            [chars, S, CL],
%            [cons, CL, 'c', _]
%        ]
%    ]
%))).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Test helpers listed here
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clean_slate_unilog :-
    retractall(unilog(_, _, _, _)),
    consult('unilog.pl').

test(Predicate) :-
    write(">>>> TEST STARTING: "),
    write(Predicate),
    nl,
    Predicate.

test_case(Predicate) :-
    write(">>>> TEST STARTING:     "),
    write(Predicate),
    nl,
    clean_slate_unilog,
    Predicate.

query_unifies(Guide, Theorem) :-
    query(Guide, QueryResult),
    unify(QueryResult, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Test cases listed here
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    tc_clean_slate_unilog_0 :-
        axiom([], a0, thm),
        \+ axiom([], a0, anythingelse).

    tc_clean_slate_unilog_1 :-
        axiom([], a0, anythingelse).

test_clean_slate_unilog :-
    test_case(tc_clean_slate_unilog_0),
    test_case(tc_clean_slate_unilog_1).

    tc_fact_0 :-
        axiom([], a0, thm0),
        query_unifies(a0, thm0).

    tc_fact_1 :-
        axiom([], a1, [thm0, thm1]),
        query_unifies(a1, [thm0, thm1]).

    tc_fact_2 :-
        axiom([], a2, [_, abc, []]),
        query_unifies(a2, [_, abc, []]).

    tc_fact_3 :-
        axiom([], a3, []),
        query_unifies(a3, []).

    tc_fact_4 :-
        axiom([], a4, 'quoted atom'),
        query_unifies(a4, 'quoted atom').

    tc_fact_expect_failure_0 :-
        \+ query(nonexistenttag, _).

    tc_fact_expect_failure_1 :-
        \+ query([], _).

    tc_fact_expect_failure_2 :-
        axiom([], a0, thm0),
        query_unifies(a0, thm0),
        \+ query(nonexistenttag, _).

test_facts :-
    test_case(tc_fact_0),
    test_case(tc_fact_1),
    test_case(tc_fact_2),
    test_case(tc_fact_3),
    test_case(tc_fact_4),
    test_case(tc_fact_expect_failure_0),
    test_case(tc_fact_expect_failure_1),
    test_case(tc_fact_expect_failure_2).

% run main test
unit_test_main :-
    test(test_clean_slate_unilog),
    test(test_facts).

:- test(unit_test_main).
