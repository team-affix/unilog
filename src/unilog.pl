% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

%bscope([claims, S, Internal], [S|NextBScope], Descoped) :-
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
    query([], ModulePath, Guide, Theorem),
    decl_theorem(ModulePath, Tag, Theorem).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(Guide, Theorem) :-
    query([], [], Guide, Theorem).

%%%%%%%%%%%%%%%%%%%%%%%%%%%% scope handling

query(BStack, DStack, [bout, S, NextGuide], [claims, S, Internal]) :-
    %bscope(ScopedSExpr, [S], Internal),
    query([S|BStack], DStack,  NextGuide, Internal).

query([S|BStack], DStack, [bin, S, NextGuide], Internal) :-
    %bscope(ScopedSExpr, [S], Internal),
    query(BStack, DStack, NextGuide, [claims, S, Internal]).

query(BStack, DStack, [dout, S, NextGuide], Theorem) :-
    append(NewBStack, [S], BStack),
    query(NewBStack, [S|DStack], NextGuide, Theorem).

query(BStack, [S|DStack], [din, S, NextGuide], Theorem) :-
    append(BStack, [S], NewBStack),
    query(NewBStack, DStack, NextGuide, Theorem).

%%%%%%%%%%%%%%%%%%%%% less fundamental ROIs

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

query(BStack, DStack, [eval], [eval, Guide]) :-
    query(BStack, DStack, Guide, _).

query(BStack, DStack, [fail], [fail, Guide]) :-
    \+ query(BStack, DStack, Guide, _).

% logic ROI

query(BStack, DStack, [mp, ImpGuide, JusGuide], Y) :-
    query(BStack, DStack, ImpGuide, [if, Y, X]),
    query(BStack, DStack, JusGuide, X).

% terminal ROI

query([], DStack, [theorem, AxiomTag], Theorem) :-
    theorem(DStack, AxiomTag, Theorem).

query(BStack, DStack, [guide, GuideTag], Theorem) :-
    guide(DStack, GuideTag, Redirect),
    query(BStack, DStack, Redirect, Theorem).

:- dynamic theorem/3.
:- dynamic guide/3.

wipe_database :-
    retractall(theorem(_, _, _)),
    retractall(guide(_, _, _)),
    !.

test_case(Predicate) :-
    write(">>>> TEST STARTING:     "),
    write(Predicate),
    nl,
    wipe_database,
    call(Predicate).

test_case_0 :-
    decl_theorem([natalie, daniel], a0, [if, y, x]),
    decl_theorem([natalie, daniel], a1, x),
    query(
            [bout, daniel, [bout, natalie,
            [dout, daniel, [dout, natalie,
                [mp, [theorem, a0], [theorem, a1]]
            ]]]], _).

:-
    test_case(test_case_0),
    wipe_database. % do a terminal db wipe
