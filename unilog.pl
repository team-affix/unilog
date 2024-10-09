% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

without_last([_], []) :- !.
without_last([X|Rest], [X|RestWithoutLast]) :-
    without_last(Rest, RestWithoutLast),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

universal(X) :-
    ground(X),
    u_case(X).

u_case([]).
u_case(if).
u_case(and).
u_case(or).
u_case(cons).
u_case(believe).

unify(X, Y) :-
    unify([], [], X, Y).

% standard unification of two expressions
unify([], [], Expr, Expr) :-
    !.
% empty list is a universal symbol
unify(_, _, Expr, Expr) :-
    universal(Expr),
    !.
% pop from scope
unify([_|XScopeRest], YScope, back:X, Y) :-
    !,
    unify(XScopeRest, YScope, X, Y).
unify(XScope, [_|YScopeRest], X, back:Y) :-
    !,
    unify(XScope, YScopeRest, X, Y).
unify([S|XScopeRest], YScope, X, S:Y) :-
    !,
    unify(XScopeRest, YScope, X, Y).
unify(XScope, [S|YScopeRest], S:X, Y) :-
    !,
    unify(XScope, YScopeRest, X, Y).
% element-wise unify lists
unify(XScope, YScope, [XH|XT], [YH|YT]) :-
    !,
    unify(XScope, YScope, XH, YH),
    unify(XScope, YScope, XT, YT).
% push onto scope
unify([], YScope, X, SY) :-
    nonvar(SY), % we should never just 'generate' a scope out of thin air.
    SY = S:Y,
    !,
    append(YScope, [S], NewYScope),
    unify([], NewYScope, X, Y).
unify(XScope, [], SX, Y) :-
    nonvar(SX), % we should never just 'generate' a scope out of thin air.
    SX = S:X,
    !,
    append(XScope, [S], NewXScope),
    unify(NewXScope, [], X, Y).



bscope(X, [S|NextBScope], Descoped) :-
    unify(X, [believe, S, SExpr]),
    bscope(SExpr, NextBScope, Descoped),
    !.
bscope(SExpr, [], SExpr) :-
    !.

cscope(X, [S|NextCScope], Descoped) :-
    unify(X, S:CScoped),
    cscope(CScoped, NextCScope, Descoped),
    !.
cscope(SExpr, [], SExpr) :-
    !.

cscope_all([ScopedX|ScopedRest], Scope, [DescopedX|DescopedRest]) :-
    cscope(ScopedX, Scope, DescopedX),
    cscope_all(ScopedRest, Scope, DescopedRest),
    !.

cscope_all([], _, []) :-
    !.

bscope_from_gscope([S|ScopedRest], [S|DescopedRest]) :-
    bscope_from_gscope(PreScopedRest, DescopedRest),
    cscope_all(ScopedRest, [S], PreScopedRest),
    !.

bscope_from_gscope([], []) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

decl_axiom(ModuleID, Tag, Theorem) :-
    atomic(ModuleID),
    atomic(Tag),
    \+ clause(axiom(ModuleID, Tag, _), _),
    assertz((
        axiom(ModuleID, Tag, Theorem)
    )).

decl_guide(ModuleID, Tag, Args, Redirect) :-
    atomic(ModuleID),
    atomic(Tag),
    is_list(Args),
    \+ clause(guide(ModuleID, Tag, _, _), _),
    assertz((
        guide(ModuleID, Tag, Args, Redirect)
    )).

decl_refer(CurrentModuleID, Tag, IncomingModuleID) :-
    atomic(CurrentModuleID),
    atomic(Tag),
    atomic(IncomingModuleID),
    \+ clause(refer(CurrentModuleID, Tag, _), _),
    \+ clause(refer(IncomingModuleID, back, _), _),
    assertz((
        refer(CurrentModuleID, Tag, IncomingModuleID)
    )),
    assertz((
        refer(IncomingModuleID, back, CurrentModuleID)
    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(ModuleID, Guide, Theorem) :-
    query(ModuleID, [], [], Guide, Theorem).

% OUTERMOST WRAPPER FOR query()
query(ModuleID, GStack, BStack, Guide, Theorem) :-
    clause(query_case(ModuleID, GStack, TargetBStack, Guide, TargetTheorem), _),
    unify(BStack, TargetBStack), % do custom unification
    unify(Theorem, TargetTheorem),
    query_case(ModuleID, GStack, TargetBStack, Guide, TargetTheorem).

query_case(ModuleID, GStack, BStack, [bout, S, NextGuide], ScopedSExpr) :-
    cscope(CScopedS, GStack, S),
    bscope(ScopedSExpr, [CScopedS], Internal),
    append(BStack, [CScopedS], NewBStack),
    query(ModuleID, GStack, NewBStack, NextGuide, Internal).

query_case(ModuleID, GStack, BStack, [bin, S, NextGuide], Internal) :-
    cscope(CScopedS, GStack, S),
    bscope(ScopedSExpr, [CScopedS], Internal),
    append(NewBStack, [CScopedS], BStack),
    query(ModuleID, GStack, NewBStack, NextGuide, ScopedSExpr).

query_case(ModuleID, GStack, BStack, [gout, S, NextGuide], ScopedSExpr) :-
    clause(refer(ModuleID, S, NextModuleID), _),
    
    % main functionality
    cscope(ScopedSExpr, [S], Internal),
    
    % gstack functionality
    append(GStack, [S], AppendedGStack),
    cscope_all(NewGStack, [back], AppendedGStack),
    
    % bstack functionality
    cscope_all(NewBStack, [back], BStack),

    query(NextModuleID, NewGStack, NewBStack, NextGuide, Internal).

query_case(ModuleID, GStack, BStack, [gin, S, NextGuide], Internal) :-
    clause(refer(ModuleID, back, NextModuleID), _),
    
    % main functionality
    cscope(ScopedSExpr, [S], Internal),

    % gstack functionality
    cscope_all(GStack, [back], UnprefixedGStack),
    append(NewGStack, [S], UnprefixedGStack),

    % bstack functionality
    cscope_all(NewBStack, [S], BStack),
    
    query(NextModuleID, NewGStack, NewBStack, NextGuide, ScopedSExpr).

% logic ROI

query_case(ModuleID, GStack, BStack, [mp, ImpGuide, JusGuide], Y) :-
    query(ModuleID, GStack, BStack, ImpGuide, [if, Y, X]),
    query(ModuleID, GStack, BStack, JusGuide, X).

query_case(ModuleID, GStack, BStack, [axiom, AxiomTag], Theorem) :-
    clause(axiom(ModuleID, AxiomTag, TargetTheorem), _),
    unify(Theorem, TargetTheorem),
    bscope_from_gscope(BStack, GStack).

query_case(ModuleID, GStack, BStack, [guide, GuideTag], Theorem) :-
    guide(ModuleID, GuideTag, Redirect),
    query(ModuleID, GStack, BStack, Redirect, Theorem).

:- dynamic axiom/3.
:- dynamic guide/4.
:- dynamic refer/3.

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
