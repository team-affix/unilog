:- consult('unilog.pl').
:- multifile unilog/2.

%%%%%%%%%%%%%%%%%%%%%%%
% DECLARATIONS (axioms and guides)
%%%%%%%%%%%%%%%%%%%%%%%

unilog(guide([], a0), fact(theorem(thm0))).
unilog(guide([], a1), fact(theorem([thm0, thm1]))).
unilog(guide([], a2), fact(theorem([_, abc, []]))).
unilog(guide([], a3), fact(theorem([]))).
unilog(guide([], a4), fact(theorem('quoted atom'))).

unilog(guide([], mp_imp_0), fact(theorem([if, consequent, antecedent]))).
unilog(guide([], mp_jus_0), fact(theorem(antecedent))).

unilog(guide([], mp_imp_1), fact(theorem([if, consequent1, [and, jus0, jus1]]))).
unilog(guide([], mp_jus_1), fact(theorem())).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Test helpers listed here
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

test(Predicate) :-
    write(">>>> TEST STARTING: "),
    write(Predicate),
    nl,
    Predicate.

test_case(Predicate) :-
    write(">>>> TEST STARTING:     "),
    write(Predicate),
    nl,
    Predicate.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Test cases listed here
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

tc_query_fact_in_root_namespace_0 :-
    query_entry(a0, R),
    R = thm0.

tc_query_fact_in_root_namespace_1 :-
    query_entry(a1, R),
    R = [thm0, thm1].

tc_query_fact_in_root_namespace_2 :-
    query_entry(a2, R),
    R = [_, abc, []].

tc_query_fact_in_root_namespace_3 :-
    query_entry(a3, R),
    R = [].

tc_query_fact_in_root_namespace_4 :-
    query_entry(a4, R),
    R = 'quoted atom'.

tc_query_fact_in_root_namespace_expect_failure_0 :-
    \+ query_entry(nonexistenttag, _).

tc_query_fact_in_root_namespace_expect_failure_1 :-
    \+ query_entry([], _).

test_query_facts_in_root_namespace :-
    test_case(tc_query_fact_in_root_namespace_0),
    test_case(tc_query_fact_in_root_namespace_1),
    test_case(tc_query_fact_in_root_namespace_2),
    test_case(tc_query_fact_in_root_namespace_3),
    test_case(tc_query_fact_in_root_namespace_4),
    test_case(tc_query_fact_in_root_namespace_expect_failure_0),
    test_case(tc_query_fact_in_root_namespace_expect_failure_1).

tc_query_conj_in_root_namespace_0 :-
    query_entry([conj, a0, a1], R),
    R = [and, thm0, [thm0, thm1]].

tc_query_conj_in_root_namespace_1 :-
    query_entry([conj, a0, a2], R),
    R = [and, thm0, [_, abc, []]].

tc_query_conj_in_root_namespace_2 :-
    query_entry([conj, a1, a2], R),
    R = [and, [thm0, thm1], [_, abc, []]].

tc_query_conj_in_root_namespace_3 :-
    query_entry([conj, a1, a4], R),
    R = [and, [thm0, thm1], 'quoted atom'].

tc_query_conj_in_root_namespace_multiple_args_0 :-
    query_entry([conj, a0, a1, a2, a3, a4], R),
    R = [and, thm0, [thm0, thm1], [_, abc, []], [], 'quoted atom'].

tc_query_conj_in_root_namespace_multiple_args_1 :-
    query_entry([conj, a0, a1, a4, a3, a2], R),
    R = [and, thm0, [thm0, thm1], 'quoted atom', [], [_, abc, []]].

test_query_conj_in_root_namespace :-
    test_case(tc_query_conj_in_root_namespace_0),
    test_case(tc_query_conj_in_root_namespace_1),
    test_case(tc_query_conj_in_root_namespace_2),
    test_case(tc_query_conj_in_root_namespace_3),
    test_case(tc_query_conj_in_root_namespace_multiple_args_0),
    test_case(tc_query_conj_in_root_namespace_multiple_args_1).


tc_query_mp_in_root_namespace_0 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_query_mp_in_root_namespace_1 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_query_mp_in_root_namespace_2 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_query_mp_in_root_namespace_3 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

test_query_mp_in_root_namespace :-
    test_case(tc_query_mp_in_root_namespace_0),
    test_case(tc_query_mp_in_root_namespace_1),
    test_case(tc_query_mp_in_root_namespace_2),
    test_case(tc_query_mp_in_root_namespace_3).

% run main test
unit_test_main :-
    test(test_query_facts_in_root_namespace),
    test(test_query_conj_in_root_namespace),
    test(test_query_mp_in_root_namespace).

:- test(unit_test_main).
