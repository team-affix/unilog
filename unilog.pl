% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

bscope([claims, S, Internal], [S|NextBScope], Descoped) :-
    bscope(Internal, NextBScope, Descoped),
    !.
bscope(SExpr, [], SExpr) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

decl_theorem(ModulePath, Tag, Theorem) :-
    is_list(ModulePath),
    atomic(Tag),
    \+ clause(theorem(ModulePath, Tag, _), _),
    assertz((
        theorem(ModulePath, Tag, Theorem)
    )).

decl_guide(ModulePath, Tag, Args, Redirect) :-
    is_list(ModulePath),
    atomic(Tag),
    is_list(Args),
    \+ clause(guide(ModulePath, Tag, _, _), _),
    assertz((
        guide(ModulePath, Tag, Args, Redirect)
    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(Guide, Theorem) :-
    query([], [], Guide, Theorem).

query(DStack, BStack, [bout, S, NextGuide], ScopedSExpr) :-
    append(BStack, [S], NewBStack),
    bscope(ScopedSExpr, [S], Internal),
    query(DStack, NewBStack, NextGuide, Internal).

query(DStack, BStack, [bin, S, NextGuide], Internal) :-
    append(NewBStack, [S], BStack),
    bscope(ScopedSExpr, [S], Internal),
    query(DStack, NewBStack, NextGuide, ScopedSExpr).

query(DStack, [S|NewBStack], [dout, S, NextGuide], Theorem) :-
    append(DStack, [S], NewDStack),
    query(NewDStack, NewBStack, NextGuide, Theorem).

query(DStack, BStack, [din, S, NextGuide], Theorem) :-
    append(NewDStack, [S], DStack),
    query(NewDStack, [S|BStack], NextGuide, Theorem).

%%%%%%%%%%%%%%%%%%%%% less fundamental ROIs

query(DStack, BStack, [bind, Target, NextGuide], Target) :-
    query(DStack, BStack, NextGuide, Target).

query(DStack, BStack, [sub, SubTarget, SubGuide, NextGuide], Target) :-
    query(DStack, BStack, SubGuide, SubTarget),
    query(DStack, BStack, NextGuide, Target).

query(DStack, BStack, [cond, [[CondTheorem|CondGuide]|NextGuide] | CondsRest], Target) :-
    query(DStack, BStack, CondGuide, CondTheorem),
    !, % if condition goal succeeds, do NOT try any other branches
    query(DStack, BStack, NextGuide, Target)
    ;
    query(DStack, BStack, [cond | CondsRest], Target).

% logic ROI

query(DStack, BStack, [mp, ImpGuide, JusGuide], Y) :-
    query(DStack, BStack, ImpGuide, [if, Y, X]),
    query(DStack, BStack, JusGuide, X).

% terminal ROI

query(DStack, [], [theorem, AxiomTag], Theorem) :-
    theorem(DStack, AxiomTag, Theorem).

query(DStack, BStack, [guide, GuideTag], Theorem) :-
    guide(DStack, GuideTag, Redirect),
    query(DStack, BStack, Redirect, Theorem).

:- dynamic theorem/3.
:- dynamic guide/4.

%:-
%    axiom([], a0, [awesome, jake]),
%    axiom([], a1, [if, y, x]),
%    axiom([], a2, x),
%    axiom([], a3, [believe, m1, x]),
%    axiom([], a4, [believe, m1, [if, y, x]]),
%    axiom([], a5,
%        [believe, m1,
%            [if,
%                [believe, m2, b],
%                a
%            ]
%        ]),
%    axiom([], a6, [believe, m1, a]),
%    axiom([], a7,
%        [believe, m1,
%            [believe, m2,
%                [if,
%                    c,
%                    b
%                ]
%            ]
%        ]),
%    axiom([m3], a0, [believe, m1, [if, y, x]]),
%    axiom([m3], a1, [believe, m1, x])
%    .
%
%:-
%    \+ query([mp, a4, a3], _),
%    query([bout, m1, [mp, [bin, m1, a4], [bin, m1, a3]]], _),
%    query([bout, m1, [mp, [bin, m1, a4], [bin, m1, a3]]], R2),
%        unify(R2, [believe, m1, y]),
%    query([bout, m1, [bout, m2, [mp, [bin, m2, [bin, m1, a7]], [bin, m2, [mp, [bin, m1, a5], [bin, m1, a6]]]]]], R3),
%        unify(R3, [believe, m1, [believe, m2, c]]),
%    query([bout, m3, [gout, m3, [bout, m1, [mp, [bin, m1, a0], [bin, m1, a1]]]]], R4),
%        unify(R4,
%            [believe, m3,
%                m3:[
%                    believe, m1, y
%                ]
%            ]),
%    query([bout, m3, [gout, m3, [bout, m1, [mp, [bin, m1, a0], [bin, m1, a1]]]]],
%        [believe, m3, [believe, m3:m1, m3:y]]
%        )
%    .
