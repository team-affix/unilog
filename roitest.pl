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

unilog(guide([], mp_imp_2), fact(theorem([if, c, [if, consequent, [and, antecedent]]]))).

unilog(guide([], a5), fact(theorem([scope, m1, [if, b, a]]))).
unilog(guide([], a6), fact(theorem([scope, m1, a]))).

unilog(guide([], a7), fact(theorem([scope, m1, [scope, m2, [if, d, c]]]))).
unilog(guide([], a8), fact(theorem([scope, m1, [scope, m2, c]]))).

unilog(guide([m1], a0), fact(theorem(a))).
unilog(guide([m1], a1), fact(theorem([if, b, a]))).
unilog(guide([m1], a2), fact(theorem([scope, m2, [if, d, c]]))).

unilog(guide([m1, m2], a0), fact(theorem(c))).
unilog(guide([m1, m2], a1), fact(theorem([if, d, c]))).



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

tc_fact_0 :-
    query_entry(a0, R),
    R = thm0.

tc_fact_1 :-
    query_entry(a1, R),
    R = [thm0, thm1].

tc_fact_2 :-
    query_entry(a2, R),
    R = [_, abc, []].

tc_fact_3 :-
    query_entry(a3, R),
    R = [].

tc_fact_4 :-
    query_entry(a4, R),
    R = 'quoted atom'.

tc_fact_expect_failure_0 :-
    \+ query_entry(nonexistenttag, _).

tc_fact_expect_failure_1 :-
    \+ query_entry([], _).

test_facts :-
    test_case(tc_fact_0),
    test_case(tc_fact_1),
    test_case(tc_fact_2),
    test_case(tc_fact_3),
    test_case(tc_fact_4),
    test_case(tc_fact_expect_failure_0),
    test_case(tc_fact_expect_failure_1).


tc_empty_discharge_cond :-
    query_entry([discharge, cond], R),
    R = [if, X, [and, X]].

tc_discharge_cond_1_condition :-
    query_entry([discharge, [mp, mp_imp_0, cond]], R),
    R = [if, consequent, [and, antecedent]].

tc_discharge_cond_empty_mp :-
    query_entry([discharge, [mp, cond, cond]], R),
    R = [if, Y, [and, [if, Y, X], X]].

tc_discharge_cond_empty_conj :-
    query_entry([discharge, [conj, cond, cond]], R),
    R = [if, [and, X, Y], [and, X, Y]].

tc_discharge_cond_nested :-
    query_entry([mp, mp_imp_2, [discharge, [mp, mp_imp_0, cond]]], R),
    R = c.

test_discharge_cond :-
    test_case(tc_empty_discharge_cond),
    test_case(tc_discharge_cond_1_condition),
    test_case(tc_discharge_cond_empty_mp),
    test_case(tc_discharge_cond_empty_conj),
    test_case(tc_discharge_cond_nested).


tc_tenter_fact_depth_1 :-
    query_entry([tenter, m1, a5], R),
    R = [scope, m1, [if, b, a]].

tc_tenter_fact_depth_2 :-
    query_entry([tenter, m1, [tenter, m2, a7]], R),
    R = [scope, m1, [scope, m2, [if, d, c]]].

tc_tenter_mp_depth_1 :-
    query_entry([tenter, m1, [mp, a5, a6]], R),
    R = [scope, m1, b].

tc_tenter_mp_depth_2 :-
    query_entry([tenter, m1, [tenter, m2, [mp, a7, a8]]], R),
    R = [scope, m1, [scope, m2, d]].

test_tenter :-
    test_case(tc_tenter_fact_depth_1),
    test_case(tc_tenter_fact_depth_2),
    test_case(tc_tenter_mp_depth_1),
    test_case(tc_tenter_mp_depth_2).


tc_genter_fact_depth_1 :-
    query_entry([genter, m1, a0], R),
    R = [scope, m1, a].

tc_genter_fact_depth_2 :-
    query_entry([genter, m1, [genter, m2, a0]], R),
    R = [scope, m1, [scope, m2, c]].

tc_genter_tenter_fact_depth_1_0 :-
    query_entry([genter, m1, [tenter, m1, a0]], R),
    R = [scope, m1, a].

% commuted version of above test
tc_genter_tenter_fact_depth_1_1 :-
    query_entry([tenter, m1, [genter, m1, a0]], R),
    R = [scope, m1, a].

tc_genter_tenter_fact_depth_2_0 :-
    query_entry([genter, m1, [genter, m2, [tenter, m1, [tenter, m2, a0]]]], R),
    R = [scope, m1, [scope, m2, c]].

tc_genter_tenter_fact_depth_2_1 :-
    query_entry([tenter, m1, [tenter, m2, [genter, m1, [genter, m2, a0]]]], R),
    R = [scope, m1, [scope, m2, c]].

tc_genter_tenter_fact_depth_2_2 :-
    query_entry([tenter, m1, [genter, m1, [tenter, m2, [genter, m2, a0]]]], R),
    R = [scope, m1, [scope, m2, c]].

tc_genter_tenter_fact_depth_2_3 :-
    query_entry([genter, m1, [tenter, m1, [genter, m2, [tenter, m2, a0]]]], R),
    R = [scope, m1, [scope, m2, c]].

tc_genter_mp_depth_1_and_depth_0 :-
    query_entry([tenter, m1, [mp, a5, [genter, m1, a0]]], R),
    R = [scope, m1, b].

tc_genter_mp_depth_1_0 :-
    query_entry([tenter, m1, [genter, m1, [mp, a1, a0]]], R),
    R = [scope, m1, b].

tc_genter_mp_depth_1_1 :-
    query_entry([genter, m1, [tenter, m1, [mp, a1, a0]]], R),
    R = [scope, m1, b].

tc_genter_mp_depth_2_and_depth_0 :-
    query_entry([tenter, m1, [tenter, m2, [mp, a7, [genter, m1, [genter, m2, a0]]]]], R),
    R = [scope, m1, [scope, m2, d]].

tc_genter_mp_depth_2_and_depth_1_0 :-
    query_entry([tenter, m1, [tenter, m2, [mp, [genter, m1, a2], [genter, m1, [genter, m2, a0]]]]], R),
    R = [scope, m1, [scope, m2, d]].

tc_genter_mp_depth_2_and_depth_1_1 :-
    query_entry([tenter, m1, [tenter, m2, [genter, m1, [mp, a2, [genter, m2, a0]]]]], R),
    R = [scope, m1, [scope, m2, d]].

tc_genter_gleave_fact_depth_1 :-
    query_entry([genter, m99, [gleave, a0]], R),
    R = thm0.

tc_genter_gleave_fact_depth_2 :-
    query_entry([genter, m99, [genter, m100, [gleave, [gleave, a0]]]], R),
    R = thm0.

tc_genter_gleave_mp_depth_2_and_0 :-
    query_entry([tenter, m1, [tenter, m2, [genter, m1, [genter, m2, [mp, a1, [gleave, [gleave, a8]]]]]]], R),
    R = [scope, m1, [scope, m2, d]].

test_genter_and_gleave :-
    test_case(tc_genter_fact_depth_1),
    test_case(tc_genter_fact_depth_2),
    test_case(tc_genter_tenter_fact_depth_1_0),
    test_case(tc_genter_tenter_fact_depth_1_1),
    test_case(tc_genter_tenter_fact_depth_2_0),
    test_case(tc_genter_tenter_fact_depth_2_1),
    test_case(tc_genter_tenter_fact_depth_2_2),
    test_case(tc_genter_tenter_fact_depth_2_3),
    test_case(tc_genter_mp_depth_1_and_depth_0),
    test_case(tc_genter_mp_depth_1_0),
    test_case(tc_genter_mp_depth_1_1),
    test_case(tc_genter_mp_depth_2_and_depth_0),
    test_case(tc_genter_mp_depth_2_and_depth_1_0),
    test_case(tc_genter_mp_depth_2_and_depth_1_1),
    test_case(tc_genter_gleave_fact_depth_1),
    test_case(tc_genter_gleave_fact_depth_2),
    test_case(tc_genter_gleave_mp_depth_2_and_0).

tc_conj_0 :-
    query_entry([conj, a0, a1], R),
    R = [and, thm0, [thm0, thm1]].

tc_conj_1 :-
    query_entry([conj, a0, a2], R),
    R = [and, thm0, [_, abc, []]].

tc_conj_2 :-
    query_entry([conj, a1, a2], R),
    R = [and, [thm0, thm1], [_, abc, []]].

tc_conj_3 :-
    query_entry([conj, a1, a4], R),
    R = [and, [thm0, thm1], 'quoted atom'].

tc_conj_multiple_args_0 :-
    query_entry([conj, a0, a1, a2, a3, a4], R),
    R = [and, thm0, [thm0, thm1], [_, abc, []], [], 'quoted atom'].

tc_conj_multiple_args_1 :-
    query_entry([conj, a0, a1, a4, a3, a2], R),
    R = [and, thm0, [thm0, thm1], 'quoted atom', [], [_, abc, []]].

test_conj :-
    test_case(tc_conj_0),
    test_case(tc_conj_1),
    test_case(tc_conj_2),
    test_case(tc_conj_3),
    test_case(tc_conj_multiple_args_0),
    test_case(tc_conj_multiple_args_1).


tc_mp_0 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_mp_1 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_mp_2 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

tc_mp_3 :-
    query_entry([mp, mp_imp_0, mp_jus_0], R),
    R = consequent.

test_mp :-
    test_case(tc_mp_0),
    test_case(tc_mp_1),
    test_case(tc_mp_2),
    test_case(tc_mp_3).

% run main test
unit_test_main :-
    test(test_facts),
    test(test_discharge_cond),
    test(test_tenter),
    test(test_genter_and_gleave),
    test(test_conj),
    test(test_mp).

:- test(unit_test_main).
