% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

%bscope([claim, S, Internal], [S|NextBScope], Descoped) :-
%    bscope(Internal, NextBScope, Descoped),
%    !.
%bscope(SExpr, [], SExpr) :-
%    !.

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
    query([], ModulePath, Guide, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% terminal ROI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query([], DStack, [t, Tag], Theorem) :-
    theorem(DStack, Tag, Theorem).

query(BStack, DStack, [r, Tag], Theorem) :-
    redir(DStack, Tag, Redirect),
    query(BStack, DStack, Redirect, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% logic ROI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [mp, ImpGuide, JusGuide], Y) :-
    query(BStack, DStack, ImpGuide, [if, Y, X]),
    query(BStack, DStack, JusGuide, X).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% more fundamental ROIs
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [bind, Target, NextGuide], Target) :-
    query(BStack, DStack, NextGuide, Target).

query(BStack, DStack, [sub, SubGuide, NextGuide], Target) :-
    query(BStack, DStack, SubGuide, _),
    query(BStack, DStack, NextGuide, Target).

query(BStack, DStack, [gor, NextGuide | Rest], Target) :-
    query(BStack, DStack, NextGuide, Target)
    ;
    query(BStack, DStack, [gor | Rest], Target).

query(BStack, DStack, [cond, [CondGuide|NextGuide] | CondsRest], Target) :-
    query(BStack, DStack, CondGuide, _),
    !, % if condition goal succeeds, do NOT try any other branches
    query(BStack, DStack, NextGuide, Target)
    ;
    query(BStack, DStack, [cond | CondsRest], Target).

query(BStack, DStack, eval, [eval, Guide]) :-
    query(BStack, DStack, Guide, _).

query(BStack, DStack, fail, [fail, Guide]) :-
    \+ query(BStack, DStack, Guide, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% conditional proofs
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% scope handling
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [bout, S, NextGuide], [claim, S, Internal]) :-
    %bscope(ScopedSExpr, [S], Internal),
    query([S|BStack], DStack,  NextGuide, Internal).

query([S|BStack], DStack, [bin, S, NextGuide], Internal) :-
    %bscope(ScopedSExpr, [S], Internal),
    query(BStack, DStack, NextGuide, [claim, S, Internal]).

query(BStack, DStack, [dout, S, NextGuide], Theorem) :-
    append(NewBStack, [S], BStack),
    query(NewBStack, [S|DStack], NextGuide, Theorem).

query(BStack, [S|DStack], [din, S, NextGuide], Theorem) :-
    append(BStack, [S], NewBStack),
    query(NewBStack, DStack, NextGuide, Theorem).

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

test_cond :-
    test_case(tc_cond_0),
    test_case(tc_cond_1),
    test_case(tc_cond_2),
    test_case(tc_cond_3),
    test_case(tc_cond_4).

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
    wipe_database. % do a terminal db wipe
