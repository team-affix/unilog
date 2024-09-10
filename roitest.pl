:- [roi8].

tc_0 :-
    query_entry(a0, R),
    R = [scope, m1, [if, y, x]].

tc_1 :-
    query_entry(a1, R),
    R = [scope, m1, x].

tc_2 :-
    query_entry([tenter, m1, a0], R),
    R = [scope, m1, [if, y, x]].

tc_3 :-
    query_entry([conj, a0, a1], R),
    R = [and,
            [scope, m1, [if, y, x]],
            [scope, m1, x]
        ].

tc_4 :-
    query_entry([tenter, m1, [conj, a0, a1]], R),
    R = [scope, m1, [and, [if, y, x], x]].

tc_5 :-
    query_entry([tenter, m1, [mp, a0, a1]], R),
    R = [scope, m1, y].

tc_6 :-
    query_entry([tenter, m1, [discharge, [mp, a0, cond]]], R),
    R = [scope, m1, [if, y, [and, x]]].

tc_7 :-
    query_entry([tenter, alg, g_last], [scope, alg, [last, [a, b, c], R]]),
    R = c.

% run main test
unit_test_main :-
    tc_0,
    tc_1,
    tc_2,
    tc_3,
    tc_4,
    tc_5,
    tc_6,
    tc_7.

:- unit_test_main.
