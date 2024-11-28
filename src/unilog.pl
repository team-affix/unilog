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

%query_all(_, [], []) :-
%    !.
%
%query_all(ModulePath, [FirstGuide|RestGuides], [FirstTheorem|RestTheorems]) :-
%    query(ModulePath, FirstGuide, FirstTheorem),
%    query_all(ModulePath, RestGuides, RestTheorems),
%    !.

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

query(BStack, DStack, Conds, [mt, ImpGuide, DenGuide], [not, X]) :-
    query(BStack, DStack, ImpConds, ImpGuide, [if, Y, X]),
    query(BStack, DStack, DenConds, DenGuide, [not, Y]),
    append(ImpConds, DenConds, Conds).

query(_, _, [], [conj], [and]).
query(BStack, DStack, Conds, [conj, FirstGuide | RestGuides], [and, FirstTheorem | RestTheorems]) :-
    query(BStack, DStack, FirstConds, FirstGuide, FirstTheorem),
    query(BStack, DStack, RestConds, [conj | RestGuides], [and | RestTheorems]),
    append(FirstConds, RestConds, Conds).

query(BStack, DStack, Conds, [disj, FirstGuide | RestGuides], [or, FirstTheorem | RestTheorems]) :-
    query(BStack, DStack, Conds, FirstGuide, FirstTheorem)
    ;
    query(BStack, DStack, Conds, [disj | RestGuides], [or | RestTheorems]).

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
    query(BStack, DStack, [], CondGuide, _),
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

    % inference fails if guide fails
    tc_infer_0 :-
        \+ infer([], i0, [mp, [t, a0], [t, a1]]),
        \+ theorem([], i0, _).

    % inference fails if guide succeeds but tag not unique
    tc_infer_1 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        decl_theorem([], i0, obstruction),
        \+ infer([], i0, [mp, [t, a0], [t, a1]]),
        theorem([], i0, R),
        R == obstruction.

    % inference succeeds if guide succeeds AND tag unique
    tc_infer_2 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        %decl_theorem([], i0, obstruction),
        infer([], i0, [mp, [t, a0], [t, a1]]),
        theorem([], i0, R),
        R == y.

    % inference succeeds requires same module stack
    tc_infer_3 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        \+ infer([m1], i0, [mp, [t, a0], [t, a1]]),
        \+ theorem([m1], i0, _).

    % inference succeeds requires same module stack
    tc_infer_4 :-
        decl_theorem([m1], a0, [if, y, x]),
        decl_theorem([m1], a1, x),
        \+ infer([], i0, [mp, [t, a0], [t, a1]]),
        \+ theorem([], i0, _).

test_infer :-
    test_case(tc_infer_0),
    test_case(tc_infer_1),
    test_case(tc_infer_2),
    test_case(tc_infer_3),
    test_case(tc_infer_4).

    % ensure query of nonexistent theorem fails
    tc_query_0 :-
        \+ query([], [t, a0], _).

    % ensure query of existing theorem succeeds
    tc_query_1 :-
        decl_theorem([], a0, x),
        query([], [t, a0], R),
        R == x.

    % module stacks need to be same
    tc_query_2 :-
        decl_theorem([], a0, x),
        \+ query([m1], [t, a0], _).

    % module stacks need to be same
    tc_query_3 :-
        decl_theorem([m1], a0, x),
        \+ query([], [t, a0], _).

test_query :-
    test_case(tc_query_0),
    test_case(tc_query_1),
    test_case(tc_query_2),
    test_case(tc_query_3).

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

    % succeeds with correct format theorems
    tc_mp_0 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, x),
        query([], [mp, [t, a0], [t, a1]], R),
        R == y.

    % produces correct format theorems
    tc_mp_1 :-
        decl_theorem([], a0, [if, [y], x]),
        decl_theorem([], a1, x),
        query([], [mp, [t, a0], [t, a1]], R),
        R == [y].

    % fails on incorrect format theorems
    tc_mp_2 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, y),
        \+ query([], [mp, [t, a0], [t, a1]], _).

    % fails on incorrect format theorems
    tc_mp_3 :-
        decl_theorem([], a0, [and, y, x]),
        decl_theorem([], a1, x),
        \+ query([], [mp, [t, a0], [t, a1]], _).

    % fails on nonpresent theorems
    tc_mp_4 :-
        decl_theorem([], a0, [if, y, x]),
        \+ query([], [mp, [t, a0], [t, a1]], _).

test_mp :-
    test_case(tc_mp_0),
    test_case(tc_mp_1),
    test_case(tc_mp_2),
    test_case(tc_mp_3),
    test_case(tc_mp_4).

    % test mt with correct format theorems
    tc_mt_0 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, [not, y]),
        query([], [mt, [t, a0], [t, a1]], R),
        R == [not, x].

    % fails on incorrect format theorems
    tc_mt_1 :-
        decl_theorem([], a0, [if, y, x]),
        decl_theorem([], a1, [not, x]),
        \+ query([], [mt, [t, a0], [t, a1]], _).

    % fails on nonexistent theorems
    tc_mt_2 :-
        decl_theorem([], a0, [if, y, x]),
        \+ query([], [mt, [t, a0], [t, a1]], _).

test_mt :-
    test_case(tc_mt_0),
    test_case(tc_mt_1),
    test_case(tc_mt_2).

    % conj base case succeeds
    tc_conj_0 :-
        query([], [conj], R),
        R == [and].

    % conj base case succeeds in other module scope
    tc_conj_1 :-
        query([m1], [conj], R),
        R == [and].
        
    % conj fails when first subguide fails
    tc_conj_2 :-
        \+ query([], [conj, [t, a0]], _).
        
    % conj fails when second subguide fails
    tc_conj_3 :-
        decl_theorem([], a0, a),
        %decl_theorem([], a1, b),
        \+ query([], [conj, [t, a0], [t, a1]], _).
        
    % conj succeeds only when all subguides succeed
    tc_conj_4 :-
        decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        query([], [conj, [t, a0], [t, a1]], R),
        R == [and, a, b].

test_conj :-
    test_case(tc_conj_0),
    test_case(tc_conj_1),
    test_case(tc_conj_2),
    test_case(tc_conj_3),
    test_case(tc_conj_4).

    % ensure base case of disj fails (identity is zero)
    tc_disj_0 :-
        \+ query([], [disj], _).

    % fails if ALL subguides fail
    tc_disj_1 :-
        \+ query([], [disj, [t, a0], [t, a1]], _).

    % succeeds if first subguide succeeds
    tc_disj_2 :-
        decl_theorem([], a0, a),
        query([], [disj, [t, a0], [t, a1]], R),
        R =@= [or, a | _].

    % succeeds if second subguide succeeds
    tc_disj_3 :-
        decl_theorem([], a1, b),
        query([], [disj, [t, a0], [t, a1]], R),
        R =@= [or, _, b | _].

    % succeeds if second subguide succeeds, and produces finite theorem
    tc_disj_4 :-
        decl_theorem([], a1, b),
        query([], [disj, [t, a0], [t, a1]], [or, _, R]),
        R == b.

test_disj :-
    test_case(tc_disj_0),
    test_case(tc_disj_1),
    test_case(tc_disj_2),
    test_case(tc_disj_3),
    test_case(tc_disj_4).

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

    tc_scope_0 :-
        scope(R, [], x),
        R == x.

    tc_scope_1 :-
        scope(R, [m1], x),
        R == [claim, m1, x].

    tc_scope_2 :-
        scope(x, [], R),
        R == x.

    tc_scope_3 :-
        scope([claim, m1, x], [m1], R),
        R == x.

    tc_scope_4 :-
        scope([claim, m1, x], S, R),
        S == [m1],
        R == x.

    tc_scope_5 :-
        scope(R, [m1, m2], a),
        R == [claim, m1, [claim, m2, a]].

test_scope :-
    test_case(tc_scope_0),
    test_case(tc_scope_1),
    test_case(tc_scope_2),
    test_case(tc_scope_3),
    test_case(tc_scope_4),
    test_case(tc_scope_5).

    tc_scope_all_0 :-
        scope_all(R, [], []),
        R == [].

    tc_scope_all_1 :-
        scope_all(R, [m1], []),
        R == [].

    tc_scope_all_2 :-
        scope_all(R, [], [a, b]),
        R == [a, b].

    tc_scope_all_3 :-
        scope_all(R, [m1], [a, b]),
        R == [[claim, m1, a], [claim, m1, b]].

    tc_scope_all_4 :-
        scope_all([a, b], [], R),
        R == [a, b].

    tc_scope_all_5 :-
        scope_all([[claim, m1, a], [claim, m1, b]], [], R),
        R == [[claim, m1, a], [claim, m1, b]].

    tc_scope_all_6 :-
        scope_all([[claim, m1, a], [claim, m1, b]], [m1], R),
        R == [a, b].

    tc_scope_all_7 :-
        scope_all(R, [m1, m2], [a, b]),
        R == [[claim, m1, [claim, m2, a]], [claim, m1, [claim, m2, b]]].

test_scope_all :-
    test_case(tc_scope_all_0),
    test_case(tc_scope_all_1),
    test_case(tc_scope_all_2),
    test_case(tc_scope_all_3),
    test_case(tc_scope_all_4),
    test_case(tc_scope_all_5),
    test_case(tc_scope_all_6),
    test_case(tc_scope_all_7).

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

    % failed to discharge assumptions
    tc_discharge_assume_0 :-
        decl_theorem([], a0, [if, y, x]),
        \+ query([], [mp, [t, a0], assume], _).

    % discharge single assumption (justification of mp)
    tc_discharge_assume_1 :-
        decl_theorem([], a0, [if, y, x]),
        query([], [discharge, [mp, [t, a0], assume]], R),
        R == [if, y, [and, x]].

    % discharge single assumption (implication of mp)
    tc_discharge_assume_2 :-
        decl_theorem([], a0, x),
        query([], [discharge, [mp, assume, [t, a0]]], R),
        R =@= [if, X, [and, [if, X, x]]].

    % test discharge/assume with changing scopes (discharge outermost op)
    tc_discharge_assume_3 :-
        decl_theorem([], a0, [claim, m1, [if, b, a]]),
        query([], 
        [discharge,
            [bout, m1,
                [mp,
                    [bin, m1, [t, a0]],
                    assume
                ]
            ]
        ], R),
        R == [if, [claim, m1, b], [and, [claim, m1, a]]].

    % test discharge/assume with changing scopes (bout outermost op)
    tc_discharge_assume_4 :-
        decl_theorem([], a0, [claim, m1, [if, b, a]]),
        query([], 
        [bout, m1,
            [discharge,
                [mp,
                    [bin, m1, [t, a0]],
                    assume
                ]
            ]
        ], R),
        R == [claim, m1, [if, b, [and, a]]].

    % test discharge/assume with changing scopes, multiple scopes (discharge outermost op)
    tc_discharge_assume_5 :-
        decl_theorem([], a0, [claim, m1, [claim, m2, [if, b, a]]]),
        query([],
        [discharge,
            [bout, m1,
                [bout, m2,
                    [mp,
                        [bin, m2, [bin, m1, [t, a0]]],
                        assume
                    ]
                ]
            ]
        ], R),
        R == [if, [claim, m1, [claim, m2, b]], [and, [claim, m1, [claim, m2, a]]]].

    % test discharge/assume with multiple conditions
    tc_discharge_assume_6 :-
        query([],
        [discharge,
            [mp, assume, assume]
        ], R),
        R =@= [if, Y, [and, [if, Y, X], X]].

    % test discharge/assume with multiple conditions, and a scope (no bin calls necessary)
    tc_discharge_assume_7 :-
        query([],
        [discharge,
            [bout, m1, [mp, assume, assume]]
        ], R),
        R =@= [if, [claim, m1, Y], [and, [claim, m1, [if, Y, X]], [claim, m1, X]]].

    % discharge/assume under bind
    tc_discharge_assume_8 :-
        query([], [discharge, [bind, B, [mp, assume, assume]]], R),
        B =@= _,
        R =@= [if, Y, [and, [if, Y, X], X]].

    % discharge/assume under sub (no assumptions in subguide)
    tc_discharge_assume_9 :-
        decl_theorem([], a0, x),
        query([], [discharge, [sub, [t, a0], [mp, assume, assume]]], R),
        R =@= [if, Y, [and, [if, Y, X], X]].

    % discharge/assume under sub (undischarged assumptions in subguide) (subguide is different query, thus should be discharged)
    tc_discharge_assume_10 :-
        decl_theorem([], a0, x),
        \+ query([], [discharge, [sub, assume, [mp, assume, assume]]], _).

    % discharge/assume under gor (first branch succeeds)
    tc_discharge_assume_11 :-
        decl_theorem([], a0, a),
        %decl_theorem([], a1, b),
        query([], [discharge, [gor, [mp, assume, [t, a0]], [mp, assume, [t, a1]]]], R),
        R =@= [if, Y, [and, [if, Y, a]]].

    % discharge/assume under gor (second branch succeeds)
    tc_discharge_assume_12 :-
        %decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        query([], [discharge, [gor, [mp, assume, [t, a0]], [mp, assume, [t, a1]]]], R),
        R =@= [if, Y, [and, [if, Y, b]]].

    % discharge/assume under cond (first branch taken)
    tc_discharge_assume_13 :-
        decl_theorem([], a0, a),
        %decl_theorem([], a1, b),
        decl_theorem([], indic0, x),
        %decl_theorem([], indic1, x),
        query([], 
        [discharge, 
            [cond,
                [[t, indic0] | [mp, assume, [t, a0]]],
                [[t, indic1] | [mp, assume, [t, a1]]]
            ]
        ], R),
        R =@= [if, Y, [and, [if, Y, a]]].

    % discharge/assume under cond (second branch taken)
    tc_discharge_assume_14 :-
        %decl_theorem([], a0, a),
        decl_theorem([], a1, b),
        %decl_theorem([], indic0, x),
        decl_theorem([], indic1, x),
        query([], 
        [discharge, 
            [cond,
                [[t, indic0] | [mp, assume, [t, a0]]],
                [[t, indic1] | [mp, assume, [t, a1]]]
            ]
        ], R),
        R =@= [if, Y, [and, [if, Y, b]]].

    % eval test, ensure that assumption list is NOT SHARED between caller and callee
    tc_discharge_assume_15 :-
        decl_theorem([], a0, a),
        \+ query([],
        [discharge, eval], [if, [eval, assume], [and|_]]),
        query([],
        [discharge, eval], [if, [eval, [t, a0]], [and|X]]),
        X == [].

    % fail test, ensure that assumption list is NOT SHARED between caller and callee
    tc_discharge_assume_16 :-
        decl_theorem([], a0, a),
        query([],
        [discharge, fail], [if, [fail, assume], [and|X]]),
        X == []. % ensure no conditions transfer

    % discharge/cond under mt, only denial is assumed
    tc_discharge_assume_17 :-
        decl_theorem([], a0, [if, b, a]),
        query([], [discharge, [mt, [t, a0], assume]], R),
        R == [if, [not, a], [and, [not, b]]].

    % discharge/cond under mt, only implication is assumed
    tc_discharge_assume_18 :-
        decl_theorem([], a0, [not, b]),
        query([], [discharge, [mt, assume, [t, a0]]], R),
        R =@= [if, [not, X], [and, [if, b, X]]].

    % discharge/cond under mt, both parts assumed
    tc_discharge_assume_19 :-
        query([], [discharge, [mt, assume, assume]], R),
        R =@= [if, [not, X], [and, [if, Y, X], [not, Y]]].

    % discharge/cond under conj (1 subguide)
    tc_discharge_assume_20 :-
        query([], [discharge, [conj, assume]], R),
        R =@= [if, [and, X], [and, X]].

    % discharge/cond under conj (2 subguides, only 1 assumption)
    tc_discharge_assume_21 :-
        decl_theorem([], a0, a),
        query([], [discharge, [conj, assume, [t, a0]]], R),
        R =@= [if, [and, X, a], [and, X]].

    % discharge/cond under conj (2 subguides, 2nd is assumed)
    tc_discharge_assume_22 :-
        decl_theorem([], a0, a),
        query([], [discharge, [conj, [t, a0], assume]], R),
        R =@= [if, [and, a, X], [and, X]].

    % discharge/cond under conj (2 subguides, both are assumed)
    tc_discharge_assume_23 :-
        query([], [discharge, [conj, assume, assume]], R),
        R =@= [if, [and, X, Y], [and, X, Y]].

    % test under disj, 1 subguide, 1st assumed
    tc_discharge_assume_24 :-
        query([], [discharge, [disj, assume]], R),
        R =@= [if, [or, X | _], [and, X]].

    % test under disj, 2 subguides, 1st assumed
    tc_discharge_assume_25 :-
        query([], [discharge, [disj, assume, [t, a0]]], R),
        R =@= [if, [or, X | _], [and, X]].

    % test under disj, 2 subguides, 2nd assumed (only one branch is ever taken)
    tc_discharge_assume_26 :-
        query([], [discharge, [disj, [t, a0], assume]], R),
        R =@= [if, [or, _, X | _], [and, X]].

test_discharge_assume :-
    test_case(tc_discharge_assume_0),
    test_case(tc_discharge_assume_1),
    test_case(tc_discharge_assume_2),
    test_case(tc_discharge_assume_3),
    test_case(tc_discharge_assume_4),
    test_case(tc_discharge_assume_5),
    test_case(tc_discharge_assume_6),
    test_case(tc_discharge_assume_7),
    test_case(tc_discharge_assume_8),
    test_case(tc_discharge_assume_9),
    test_case(tc_discharge_assume_10),
    test_case(tc_discharge_assume_11),
    test_case(tc_discharge_assume_12),
    test_case(tc_discharge_assume_13),
    test_case(tc_discharge_assume_14),
    test_case(tc_discharge_assume_15),
    test_case(tc_discharge_assume_16),
    test_case(tc_discharge_assume_17),
    test_case(tc_discharge_assume_18),
    test_case(tc_discharge_assume_19),
    test_case(tc_discharge_assume_20),
    test_case(tc_discharge_assume_21),
    test_case(tc_discharge_assume_22),
    test_case(tc_discharge_assume_23),
    test_case(tc_discharge_assume_24),
    test_case(tc_discharge_assume_25),
    test_case(tc_discharge_assume_26).

:-
    test(test_wipe_database),
    test(test_decl_theorem),
    test(test_decl_redir),
    test(test_infer),
    test(test_query),
    test(test_t),
    test(test_r),
    test(test_mp),
    test(test_mt),
    test(test_conj),
    test(test_disj),
    test(test_bind),
    test(test_sub),
    test(test_gor),
    test(test_cond),
    test(test_eval),
    test(test_fail),
    test(test_scope),
    test(test_scope_all),
    test(test_bout),
    test(test_dout),
    test(test_discharge_assume),
    wipe_database. % do a terminal db wipe
