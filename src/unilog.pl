% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

%bscope([claim, S, Internal], [S|NextBScope], Descoped) :-
%    bscope(Internal, NextBScope, Descoped),
%    !.
%bscope(SExpr, [], SExpr) :-
%    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

decl_theorem(ModulePath, Tag, Theorem) :-
    is_list(ModulePath),
    atomic(Tag),
    \+ clause(theorem(ModulePath, Tag, _), _),
    assertz((
        theorem(ModulePath, Tag, Theorem)
    )).

decl_guide(ModulePath, Tag, Redirect) :-
    is_list(ModulePath),
    \+ clause(guide(ModulePath, Tag, _, _), _),
    assertz((
        guide(ModulePath, Tag, Redirect)
    )).

infer(ModulePath, Tag, Guide) :-
    query(ModulePath, Guide, Theorem),
    decl_theorem(ModulePath, Tag, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Handle querying
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(ModulePath, Guide, Theorem) :-
    query([], ModulePath, Guide, Theorem).

% terminal ROI

query([], DStack, [t, Tag], Theorem) :-
    theorem(DStack, Tag, Theorem).

query(BStack, DStack, [r, Tag], Theorem) :-
    guide(DStack, Tag, Redirect),
    query(BStack, DStack, Redirect, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% logic ROI
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [mp, ImpGuide, JusGuide], Y) :-
    query(BStack, DStack, ImpGuide, [if, Y, X]),
    query(BStack, DStack, JusGuide, X).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%% less fundamental ROIs 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

query(BStack, DStack, [bind, Target, NextGuide], Target) :-
    query(BStack, DStack, NextGuide, Target).

query(BStack, DStack, [sub, SubTarget, SubGuide, NextGuide], Target) :-
    query(BStack, DStack, SubGuide, SubTarget),
    query(BStack, DStack, NextGuide, Target).

query(BStack, DStack, [cond, [[CondTheorem|CondGuide]|NextGuide] | CondsRest], Target) :-
    query(BStack, DStack, CondGuide, CondTheorem),
    !, % if condition goal succeeds, do NOT try any other branches
    query(BStack, DStack, NextGuide, Target)
    ;
    query(BStack, DStack, [cond | CondsRest], Target).

query(BStack, DStack, eval, [eval, Guide]) :-
    query(BStack, DStack, Guide, _).

query(BStack, DStack, fail, [fail, Guide]) :-
    \+ query(BStack, DStack, Guide, _).

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

:- dynamic theorem/3.
:- dynamic guide/3.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%% BEGIN UNIT TEST REGION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%
% Test helpers listed here
%%%%%%%

wipe_database :-
    retractall(theorem(_, _, _)),
    retractall(guide(_, _, _)),
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

    tc_wipe_database_0 :-
        decl_theorem([], a0, thm),
        theorem([], a0, _),
        wipe_database,
        \+ theorem([], a0, _).

    tc_wipe_database_1 :-
        decl_guide([], g0, guide),
        guide([], g0, _),
        wipe_database,
        \+ guide([], g0, _).

test_wipe_database :-
    test_case(tc_wipe_database_0),
    test_case(tc_wipe_database_1).

    tc_theorem_0 :-
        \+ query([], [t, a0], _).

    tc_theorem_1 :-
        decl_theorem([], a0, x),
        query([], [t, a0], R),
        R == x.

test_theorem :-
    test_case(tc_theorem_0),
    test_case(tc_theorem_1).

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

:-
    test(test_wipe_database),
    test(test_theorem),
    test(test_mp),
    wipe_database. % do a terminal db wipe
