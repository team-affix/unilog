% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

scope([claim, S, Internal], [S|NextBScope], Descoped) :-
    scope(Internal, NextBScope, Descoped),
    !.
scope(SExpr, [], SExpr) :-
    !.

scope_all([], _, []) :-
    !.
scope_all([SX|SRest], S, [X|Rest]) :-
    scope(SX, S, X),
    scope_all(SRest, S, Rest),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Handle theorem/guide declarations
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

:- dynamic theorem/3.
:- dynamic redir/3.

decl_theorem(ModulePath, Tag, Theorem) :-
    is_list(ModulePath),
    atomic(Tag),
    \+ clause(theorem(ModulePath, Tag, _), _),
    assertz((
        theorem(ModulePath, Tag, Theorem)
    )).

decl_redir(ModulePath, Tag, Redirect) :-
    is_list(ModulePath),
    \+ clause(redir(ModulePath, Tag, _), _),
    assertz((
        redir(ModulePath, Tag, Redirect)
    )).

infer(ModulePath, Tag, Guide) :-
    query(ModulePath, Guide, Theorem),
    decl_theorem(ModulePath, Tag, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Handle querying
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(ModulePath, Guide, Theorem) :-
    query([], ModulePath, [], Guide, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% terminal ROI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query([], DStack, [], [t, Tag], Theorem) :-
    theorem(DStack, Tag, Theorem).

query(BStack, DStack, Conds, [r, Tag], Theorem) :-
    redir(DStack, Tag, Redirect),
    query(BStack, DStack, Conds, Redirect, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% logic ROI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, Conds, [mp, ImpGuide, JusGuide], Y) :-
    query(BStack, DStack, ImpConds, ImpGuide, [if, Y, X]),
    query(BStack, DStack, JusConds, JusGuide, X),
    append(ImpConds, JusConds, Conds).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% more fundamental ROIs
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, Conds, [bind, Target, NextGuide], Target) :-
    query(BStack, DStack, Conds, NextGuide, Target).

query(BStack, DStack, Conds, [sub, SubGuide, NextGuide], Target) :-
    query(BStack, DStack, [], SubGuide, _),
    query(BStack, DStack, Conds, NextGuide, Target).

query(BStack, DStack, Conds, [gor, NextGuide | Rest], Target) :-
    query(BStack, DStack, Conds, NextGuide, Target)
    ;
    query(BStack, DStack, Conds, [gor | Rest], Target).

query(BStack, DStack, Conds, [cond, [CondGuide|NextGuide] | CondsRest], Target) :-
    query(BStack, DStack, Conds, CondGuide, _),
    !, % if condition goal succeeds, do NOT try any other branches
    query(BStack, DStack, Conds, NextGuide, Target)
    ;
    query(BStack, DStack, Conds, [cond | CondsRest], Target).

query(BStack, DStack, [], eval, [eval, Guide]) :-
    query(BStack, DStack, [], Guide, _).

query(BStack, DStack, [], fail, [fail, Guide]) :-
    \+ query(BStack, DStack, [], Guide, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% scope handling
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, Conds, [bout, S, NextGuide], Scoped) :-
    scope(Scoped, [S], Internal),
    query([S|BStack], DStack, InternalConds,  NextGuide, Internal),
    scope_all(Conds, [S], InternalConds).

query([S|BStack], DStack, Conds, [bin, S, NextGuide], Internal) :-
    scope(Scoped, [S], Internal),
    query(BStack, DStack, ScopedConds, NextGuide, Scoped),
    scope_all(ScopedConds, [S], Conds).

query(BStack, DStack, Conds, [dout, S, NextGuide], Theorem) :-
    append(NewBStack, [S], BStack),
    query(NewBStack, [S|DStack], Conds, NextGuide, Theorem).

query(BStack, [S|DStack], Conds, [din, S, NextGuide], Theorem) :-
    append(BStack, [S], NewBStack),
    query(NewBStack, DStack, Conds, NextGuide, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% conditional proofs
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [], [discharge, NextGuide], [if, Target, [and | Conds]]) :-
    query(BStack, DStack, Conds, NextGuide, Target).

query(_, _, [Target], assume, Target).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%% BEGIN UNIT TEST REGION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%
% Test helpers listed here
%%%%%%%

wipe_database :-
    retractall(theorem(_, _, _)),
    retractall(redir(_, _, _)),
    !.

test(Predicate) :-
    write(">>>> TEST STARTING: "),
    write(Predicate),
    nl,
    call(Predicate).

test_case(Predicate) :-
    write(">>>> TEST STARTING:     "),
    write(Predicate),
    nl,
    wipe_database,
    call(Predicate).

%%%%%%%
% Test cases listed here
%%%%%%%

    % make sure it wipes theorems
    tc_wipe_database_0 :-
        assertz(theorem([], a0, thm)),
        theorem([], a0, _),
        wipe_database,
        \+ theorem([], a0, _).

    % make sure it wipes redirs
    tc_wipe_database_1 :-
        assertz(redir([], g0, guide)),
        redir([], g0, _),
        wipe_database,
        \+ redir([], g0, _).

test_wipe_database :-
    test_case(tc_wipe_database_0),
    test_case(tc_wipe_database_1).

    % make sure decl_theorem calls assertz
    tc_decl_theorem_0 :-
        decl_theorem([], a0, x),
        theorem([], a0, R),
        R == x.

    % make sure theorem tags are unique
    tc_decl_theorem_1 :-
        decl_theorem([], a0, x),
        \+ decl_theorem([], a0, y).

test_decl_theorem :-
    test_case(tc_decl_theorem_0),
    test_case(tc_decl_theorem_1).

    tc_decl_redir_0 :-
        decl_redir([], r0, x),
        redir([], r0, R),
        R == x.

    tc_decl_redir_1 :-
        decl_redir([], r0, x),
        \+ decl_redir([], r0, y).

test_decl_redir :-
    test_case(tc_decl_redir_0),
    test_case(tc_decl_redir_1).

    tc_t_0 :-
        \+ query([], [t, a0], _).

    tc_t_1 :-
        decl_theorem([], a0, x),
        query([], [t, a0], R),
        R == x.

test_t :-
    test_case(tc_t_0),
    test_case(tc_t_1).

    tc_r_0 :-
        \+ query([], [r, r0], _).

    tc_r_1 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        decl_redir([], r0, [mp, [t, a0], [t, a1]]),
        query([], [r, r0], R),
        R == y.

test_r :-
    test_case(tc_r_0),
    test_case(tc_r_1).

    tc_mp_0 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        query([], [mp, [t, a0], [t, a1]], R),
        R == y.

    tc_mp_1 :-
        decl_theorem([], a0, [if, [y], x]),
        decl_theorem([], a1, x),
        query([], [mp, [t, a0], [t, a1]], R),
        R == [y].

test_mp :-
    test_case(tc_mp_0),
    test_case(tc_mp_1).

    % demonstrate bind ability to extract info from theorem
    tc_bind_0 :-
        decl_theorem([], a0, [if, y, x]),
        query([], [bind, [if, A, B], [t, a0]], R),
        R == [if, y, x],
        A == y,
        B == x.

    % demonstrate the ability to supply information using bind
    tc_bind_1 :-
        decl_theorem([], a0, [if, _, _]),
        query([], [bind, [if, y, x], [t, a0]], R),
        R == [if, y, x].

test_bind :-
    test_case(tc_bind_0),
    test_case(tc_bind_1).

    % demonstrate successful subguide
    tc_sub_0 :-
        decl_theorem([], a0, indicator),
        decl_theorem([], a1, x),
        query([], [sub, [t, a0], [t, a1]], R),
        R == x.

    % demonstrate unsuccessful subguide
    tc_sub_1 :-
        decl_theorem([], a1, x),
        \+ query([], [sub, [t, a0], [t, a1]], _).

test_sub :-
    test_case(tc_sub_0),
    test_case(tc_sub_1).

    % empty gor fails (no branches)
    tc_gor_0 :-
        \+ query([], [gor], _).

    % all branches fail
    tc_gor_1 :-
        \+ query([], [gor, [t, a0]], _).

    % only branch succeeds
    tc_gor_2 :-
        decl_theorem([], a0, x),
        query([], [gor, [t, a0]], R),
        R == x.

    % fist branch succeeds (second WOULD fail)
    tc_gor_3 :-
        decl_theorem([], a0, x),
        query([], [gor, [t, a0], [t, a1]], R),
        R == x.

    % fist branch succeeds (second WOULD NOT fail)
    tc_gor_4 :-
        decl_theorem([], a0, x),
        decl_theorem([], a1, y),
        query([], [gor, [t, a0], [t, a1]], R),
        R == x.

    % second branch succeeds (first fails)
    tc_gor_4 :-
        decl_theorem([], a1, y),
        query([], [gor, [t, a0], [t, a1]], R),
        R == x.

    % no branch succeeds, both fail
    tc_gor_5 :-
        \+ query([], [gor, [t, a0], [t, a1]], _).

test_gor :-
    test_case(tc_gor_0),
    test_case(tc_gor_1),
    test_case(tc_gor_2),
    test_case(tc_gor_3),
    test_case(tc_gor_4),
    test_case(tc_gor_5).

    % empty cond fails (no branches)
    tc_cond_0 :-
        \+ query([], [cond], _).

    % no branch is taken (no indicators reachable)
    tc_cond_1 :-
        decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        %decl_theorem([], indic0, x),
        %decl_theorem([], indic1, x),
        \+ query([],
            [cond,
                [[t, indic0]  | [t, a0]],
                [[t, indic1]  | [t, a1]]
            ], _).

    % first branch taken (first indicator reachable)
    tc_cond_2 :-
        decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        decl_theorem([], indic0, x),
        %decl_theorem([], indic1, x),
        query([],
            [cond,
                [[t, indic0]  | [t, a0]],
                [[t, indic1]  | [t, a1]]
            ], R),
        R == a.

    % first branch taken (both indicators reachable)
    tc_cond_3 :-
        decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        decl_theorem([], indic0, x),
        decl_theorem([], indic1, x),
        query([],
            [cond,
                [[t, indic0]  | [t, a0]],
                [[t, indic1]  | [t, a1]]
            ], R),
        R == a.

    % second branch taken (only second indicator)
    tc_cond_4 :-
        decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        %decl_theorem([], indic0, x),
        decl_theorem([], indic1, x),
        query([],
            [cond,
                [[t, indic0]  | [t, a0]],
                [[t, indic1]  | [t, a1]]
            ], R),
        R == b.

    % first branch taken yet NextGuide fails. Second branch is NOT TAKEN AFTER due to cut (!) (both indicators reachable)
    tc_cond_5 :-
        %decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        decl_theorem([], indic0, x),
        decl_theorem([], indic1, x),
        \+ query([],
            [cond,
                [[t, indic0]  | [t, a0]],
                [[t, indic1]  | [t, a1]]
            ], _).

test_cond :-
    test_case(tc_cond_0),
    test_case(tc_cond_1),
    test_case(tc_cond_2),
    test_case(tc_cond_3),
    test_case(tc_cond_4),
    test_case(tc_cond_5).

    % test failure of eval when subguide fails
    tc_eval_0 :-
        \+ query([], eval, [eval, [t, a0]]).

    % test success of eval, without binding result
    tc_eval_1 :-
        decl_theorem([], a0, x),
        query([], eval, [eval, [t, a0]]).

    % test success of eval, with binding result
    tc_eval_2 :-
        decl_theorem([], a0, x),
        query([], eval, [eval, [bind, R, [t, a0]]]),
        R == x.

test_eval :-
    test_case(tc_eval_0),
    test_case(tc_eval_1),
    test_case(tc_eval_2).

    % failure to find a theorem
    tc_fail_0 :-
        query([], fail, [fail, [t, a0]]).

    % finds theorem, thus 'fail' fails
    tc_fail_1 :-
        decl_theorem([], a0, x),
        \+ query([], fail, [fail, [t, a0]]).

test_fail :-
    test_case(tc_fail_0),
    test_case(tc_fail_1).

    % bout fails by not reaching subgoal
    tc_bout_0 :-
        \+ query([], [bout, m1, [bin, m1, [t, a0]]], _).

    % bout & bin, reaching theorem
    tc_bout_1 :-
        decl_theorem([], a0, [claim, m1, x]),
        query([], [bout, m1, [bin, m1, [t, a0]]], R),
        R == [claim, m1, x].

    % execute mp within m1 scope
    tc_bout_2 :-
        decl_theorem([], a0, [claim, m1, [if, y, x]]),
        decl_theorem([], a1, [claim, m1, x]),
        query([], [bout, m1, [mp,
            [bin, m1, [t, a0]],
            [bin, m1, [t, a1]]
        ]], R),
        R == [claim, m1, y].

    % execute mp within m1 scope, fail since scopes are different
    tc_bout_3 :-
        decl_theorem([], a0, [claim, m1, [if, y, x]]),
        decl_theorem([], a1, [claim, m2, x]),
        \+ query([], [bout, m1, [mp,
            [bin, m1, [t, a0]],
            [bin, m1, [t, a1]]
        ]], _).

    % nested scope test, retrieving theorem
    tc_bout_4 :-
        decl_theorem([], a0, [claim, m1, [claim, m2, x]]),
        query([], [bout, m1, [bout, m2, [bin, m2, [bin, m1, [t, a0]]]]], R),
        R == [claim, m1, [claim, m2, x]].

    % nested scope test, conducting mp
    tc_bout_5 :-
        decl_theorem([], a0, [claim, m1, [claim, m2, [if, y, x]]]),
        decl_theorem([], a1, [claim, m1, [claim, m2, x]]),
        query([],
            [bout, m1,
            [bout, m2,
                [mp,
                    [bin, m2, [bin, m1, [t, a0]]],
                    [bin, m2, [bin, m1, [t, a1]]]
                ]
            ]], R),
        R == [claim, m1, [claim, m2, y]].

test_bout :-
    test_case(tc_bout_0),
    test_case(tc_bout_1),
    test_case(tc_bout_2),
    test_case(tc_bout_3),
    test_case(tc_bout_4),
    test_case(tc_bout_5).

    % dout fails, no theorem present
    tc_dout_0 :-
        \+ query([], [bout, m1, [dout, m1, [t, a0]]], _).

    % dout fails, theorem present, no preceeding bout call
    tc_dout_1 :-
        decl_theorem([m1], a0, x),
        \+ query([], [dout, m1, [t, a0]], _).

    % bout then dout succeeds, theorem present
    tc_dout_2 :-
        decl_theorem([m1], a0, x),
        query([], [bout, m1, [dout, m1, [t, a0]]], R),
        R == [claim, m1, x].

    % mp conducted with imp and jus in same dscope
    tc_dout_3 :-
        decl_theorem([m1], a0, [if, y, x]),
        decl_theorem([m1], a1, x),
        query([], [bout, m1, [dout, m1, [mp, [t, a0], [t, a1]]]], R),
        R == [claim, m1, y].

    % mp conducted with imp and jus in different dscopes (1)
    tc_dout_4 :-
        decl_theorem([], a0, [claim, m1, [if, y, x]]),
        decl_theorem([m1], a1, x),
        query([], [bout, m1, [mp, [bin, m1, [t, a0]], [dout, m1, [t, a1]]]], R),
        R == [claim, m1, y].

    % mp conducted with imp and jus in different dscopes (2)
    tc_dout_5 :-
        decl_theorem([], a0, [claim, m1, [if, y, x]]),
        decl_theorem([m1], a1, x),
        query([], [bout, m1, [dout, m1, [mp, [din, m1, [bin, m1, [t, a0]]], [t, a1]]]], R),
        R == [claim, m1, y].

    % mp conducted with imp and jus in different dscopes (3)
    tc_dout_6 :-
        decl_theorem([m1], a0, [if, y, x]),
        decl_theorem([], a1, [claim, m1, x]),
        query([], [bout, m1, [mp, [dout, m1, [t, a0]], [bin, m1, [t, a1]]]], R),
        R == [claim, m1, y].

    % mp conducted with imp and jus in different dscopes (4)
    tc_dout_7 :-
        decl_theorem([m1], a0, [if, y, x]),
        decl_theorem([], a1, [claim, m1, x]),
        query([], [bout, m1, [dout, m1, [mp, [t, a0], [din, m1, [bin, m1, [t, a1]]]]]], R),
        R == [claim, m1, y].

test_dout :-
    test_case(tc_dout_0),
    test_case(tc_dout_1),
    test_case(tc_dout_2),
    test_case(tc_dout_3),
    test_case(tc_dout_4),
    test_case(tc_dout_5),
    test_case(tc_dout_6),
    test_case(tc_dout_7).

:-
    test(test_wipe_database),
    test(test_decl_theorem),
    test(test_decl_redir),
    test(test_t),
    test(test_r),
    test(test_mp),
    test(test_bind),
    test(test_sub),
    test(test_gor),
    test(test_cond),
    test(test_eval),
    test(test_fail),
    test(test_bout),
    test(test_dout),
    wipe_database. % do a terminal db wipe
