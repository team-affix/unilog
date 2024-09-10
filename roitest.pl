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

test_query_facts_in_root_namespace :-
    tc_query_fact_in_root_namespace_0,
    tc_query_fact_in_root_namespace_1,
    tc_query_fact_in_root_namespace_2,
    tc_query_fact_in_root_namespace_3,
    tc_query_fact_in_root_namespace_4.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% run main test
unit_test_main :-
    test_query_facts_in_root_namespace.

:- unit_test_main.
