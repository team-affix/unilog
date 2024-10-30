% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

without_last([_], []) :- !.
without_last([X|Rest], [X|RestWithoutLast]) :-
    without_last(Rest, RestWithoutLast),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

%universal(X) :-
%    ground(X),
%    u_case(X).
%
%u_case([]).
%u_case(if).
%u_case(and).
%u_case(or).
%u_case(cons).
%u_case(believe).
%
%unify(X, Y) :-
%    unify([], [], X, Y).
%
%% standard unification of two expressions
%unify([], [], Expr, Expr) :-
%    !.
%% empty list is a universal symbol
%unify(_, _, Expr, Expr) :-
%    universal(Expr),
%    !.
%% pop from scope
%unify([_|XScopeRest], YScope, back:X, Y) :-
%    !,
%    unify(XScopeRest, YScope, X, Y).
%unify(XScope, [_|YScopeRest], X, back:Y) :-
%    !,
%    unify(XScope, YScopeRest, X, Y).
%unify([S|XScopeRest], YScope, X, S:Y) :-
%    !,
%    unify(XScopeRest, YScope, X, Y).
%unify(XScope, [S|YScopeRest], S:X, Y) :-
%    !,
%    unify(XScope, YScopeRest, X, Y).
%% element-wise unify lists
%unify(XScope, YScope, [XH|XT], [YH|YT]) :-
%    !,
%    unify(XScope, YScope, XH, YH),
%    unify(XScope, YScope, XT, YT).
%% push onto scope
%unify([], YScope, X, SY) :-
%    nonvar(SY), % we should never just 'generate' a scope out of thin air.
%    SY = S:Y,
%    !,
%    append(YScope, [S], NewYScope),
%    unify([], NewYScope, X, Y).
%unify(XScope, [], SX, Y) :-
%    nonvar(SX), % we should never just 'generate' a scope out of thin air.
%    SX = S:X,
%    !,
%    append(XScope, [S], NewXScope),
%    unify(NewXScope, [], X, Y).
%
%

%full_scope(X, [], X) :-
%    (
%        X \= (_:_)
%        ;
%        var(X)
%    ),
%    !.
%
%full_scope(C:CRest, [C|SRest], Terminal) :-
%    full_scope(CRest, SRest, Terminal),
%    !.
%
%
%
%unify(X, Y) :-
%    full_scope(X, _, Terminal),
%    full_scope(Y, _, Terminal),
%    universal(Terminal),
%    !.
%
%unify(X, Y) :-
%    full_scope(X, XScope, [XH|XT]),
%    full_scope(Y, YScope, [YH|YT]),
%    % undo scope, distributing to inside
%    full_scope()

% general methodology:
%     EVERY TIME we find an expression in the form: expr(S, X), we push S onto the corresponding stack

% entry point
rescope(X, Y) :-
    rescope([], [], X, Y),
    write("entry").

% defines universal symbols
universal([]).

% base case: eq
rescope([], [], Expr, Expr) :-
    write("eq base case").

% base case: universal
rescope(_, _, Expr, Expr) :-
    \+ var(Expr),
    universal(Expr),
    write("universal").

% left push
rescope(XStack, [], X, Y) :-
    \+ var(X),
    X = scope(S, XDescoped),
    append(XStack, [S], NewXStack),
    rescope(NewXStack, [], XDescoped, Y),
    write("left push").

% right push
rescope([], YStack, X, Y) :-
    \+ var(Y),
    Y = scope(S, YDescoped),
    append(YStack, [S], NewYStack),
    rescope([], NewYStack, X, YDescoped),
    write("right push").

% left pop
rescope([S|NewXStack], [], X, Y) :-
    Y = scope(S, YDescoped),
    rescope(NewXStack, [], X, YDescoped),
    write("left pop").

% right pop
rescope([], [S|NewYStack], X, Y) :-
    X = scope(S, XDescoped),
    rescope([], NewYStack, XDescoped, Y),
    write("right pop").

% distribute into lists
rescope(XStack, YStack, X, Y) :-
    \+ var(X),\+ var(Y),
    X = [XH|XT],Y = [YH|YT],
    rescope(XStack, YStack, XH, YH),
    rescope(XStack, YStack, XT, YT),
    write("lists").







bscope(X, [S|NextBScope], Descoped) :-
    unify(X, [believe, S, SExpr]),
    bscope(SExpr, NextBScope, Descoped),
    !.
bscope(SExpr, [], SExpr) :-
    !.








cscope([S|SRest], Descoped, Scoped) :-
    cscope_case([S], OuterDescoped, Scoped),
    cscope(SRest, Descoped, OuterDescoped).

cscope([], Descoped, Scoped) :-
    cscope_case([], Descoped, Scoped).

% universal cases
cscope_case(_, X, X) :-
    (
        var(X);
        X = []
    ),
    !.

cscope_case([], X, X) :-
    !.

cscope_case(Scope, [DescopedX|DescopedRest], [X|Rest]) :-
    cscope_case(Scope, DescopedX, X),
    cscope_case(Scope, DescopedRest, Rest),
    !.

cscope_case([S|ScopeRest], Descoped, S:X) :-
    cscope_case(ScopeRest, Descoped, X),
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

decl_theorem(ModuleID, Tag, Theorem) :-
    atomic(ModuleID),
    atomic(Tag),
    \+ clause(theorem(ModuleID, Tag, _), _),
    assertz((
        theorem(ModuleID, Tag, Theorem)
    )).

decl_guide(ModuleID, Tag, Args, Redirect) :-
    atomic(ModuleID),
    atomic(Tag),
    is_list(Args),
    \+ clause(guide(ModuleID, Tag, _, _), _),
    assertz((
        guide(ModuleID, Tag, Args, Redirect)
    )).

refer(CurrentModuleID, Tag, IncomingModuleID) :-
    atomic(CurrentModuleID),
    atomic(Tag),
    atomic(IncomingModuleID),
    assertz((
        refer(CurrentModuleID, Tag, IncomingModuleID)
    )),
    assertz((
        refer(IncomingModuleID, back, CurrentModuleID)
    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(ModuleID, Guide, Theorem) :-
    query(ModuleID, [], Guide, Theorem).

query(ModuleID, BStack, [bout, S, NextGuide], ScopedSExpr) :-
    cscope(CScopedS, S),
    bscope(ScopedSExpr, [CScopedS], Internal),
    append(BStack, [CScopedS], NewBStack),
    query(ModuleID, NewBStack, NextGuide, Internal).

query(ModuleID, BStack, [bin, S, NextGuide], Internal) :-
    cscope(CScopedS, S),
    bscope(ScopedSExpr, [CScopedS], Internal),
    append(NewBStack, [CScopedS], BStack),
    query(ModuleID, NewBStack, NextGuide, ScopedSExpr).

query(ModuleID, BStack, [bind, Target, NextGuide], Target) :-
    query(ModuleID, BStack, NextGuide, Target).

query(ModuleID, BStack, [sub, SubTarget, SubGuide, NextGuide], Target) :-
    query(ModuleID, BStack, SubGuide, SubTarget),
    query(ModuleID, BStack, NextGuide, Target).

query(ModuleID, BStack, [cond, [[CondTheorem|CondGuide]|NextGuide] | CondsRest], Target) :-
    query(ModuleID, BStack, CondGuide, CondTheorem),
    !, % if condition goal succeeds, do NOT try any other branches
    query(ModuleID, BStack, NextGuide, Target)
    ;
    query(ModuleID, BStack, [cond | CondsRest], Target).

% logic ROI

query(ModuleID, BStack, [mp, ImpGuide, JusGuide], Y) :-
    query(ModuleID, BStack, ImpGuide, [if, Y, X]),
    query(ModuleID, BStack, JusGuide, X).

% terminal ROI

query(ModuleID, BStack, [theorem, AxiomTag], Theorem) :-
    theorem(ModuleID, AxiomTag, Theorem).

query(ModuleID, BStack, [guide, GuideTag], Theorem) :-
    guide(ModuleID, GuideTag, Redirect),
    query(ModuleID, BStack, Redirect, Theorem).

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
