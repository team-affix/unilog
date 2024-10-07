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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

axiom(GScope, GuideTag, Target) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(GScope, GScope, GuideTag, _), _),
    atomic(GuideTag),
    assertz((
        unilog(GScope, GScope, GuideTag, Source) :-
                % then attempt to unify.
            unify(Source, Target)
    )).

guide(GScope, GuideTag, GuideArgs, Redirect) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(GScope, GScope, GuideTag, _), _),
    atomic(GuideTag),
    is_list(GuideArgs),
    assertz((
        unilog(GScope, BScope, [GuideTag|GuideArgs], SExpr) :-
            unilog(GScope, BScope, Redirect, SExpr)
    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(Tag, SExpr) :-
    unilog([], [], Tag, SExpr).

:- dynamic unilog/4.

unilog(CScope, BScope, [bout, S, NextGuide], ScopedSExpr) :-
    bscope(ScopedSExpr, [S], Internal),
    append(BScope, [S], NewBScope),
    unilog(CScope, NewBScope, NextGuide, Internal).

unilog(CScope, BScope, [bin, S, NextGuide], Internal) :-
    bscope(ScopedSExpr, [S], Internal),
    append(NewBScope, [S], BScope),
    unilog(CScope, NewBScope, NextGuide, ScopedSExpr).

unilog(CScope, BScope, [gout, S, NextGuide], ScopedSExpr) :-
    cscope(ScopedSExpr, [S], Internal),
    append(CScope, [S], NewCScope),
    unilog(NewCScope, BScope, NextGuide, Internal).

unilog(CScope, BScope, [gin, S, NextGuide], Internal) :-
    cscope(ScopedSExpr, [S], Internal),
    append(NewCScope, [S], CScope),
    unilog(NewCScope, BScope, NextGuide, ScopedSExpr).

% logic ROI

unilog(CScope, BScope, [mp, ImpGuide, JusGuide], Y) :-
    unilog(CScope, BScope, ImpGuide, [if, Y, X]),
    unilog(CScope, BScope, JusGuide, X).

:-
    axiom([], a0, [awesome, jake]),
    axiom([], a1, [if, y, x]),
    axiom([], a2, x),
    axiom([], a3, [believe, m1, x]),
    axiom([], a4, [believe, m1, [if, y, x]]),
    axiom([], a5,
        [believe, m1,
            [if,
                [believe, m2, b],
                a
            ]
        ]),
    axiom([], a6, [believe, m1, a]),
    axiom([], a7,
        [believe, m1,
            [believe, m2,
                [if,
                    c,
                    b
                ]
            ]
        ]),
    axiom([m3], a0, [believe, m1, [if, y, x]]),
    axiom([m3], a1, [believe, m1, x])
    .

:-
    \+ query([mp, a4, a3], _),
    query([bout, m1, [mp, [bin, m1, a4], [bin, m1, a3]]], _),
    query([bout, m1, [mp, [bin, m1, a4], [bin, m1, a3]]], R2),
        unify(R2, [believe, m1, y]),
    query([bout, m1, [bout, m2, [mp, [bin, m2, [bin, m1, a7]], [bin, m2, [mp, [bin, m1, a5], [bin, m1, a6]]]]]], R3),
        unify(R3, [believe, m1, [believe, m2, c]]),
    query([bout, m3, [gout, m3, [bout, m1, [mp, [bin, m1, a0], [bin, m1, a1]]]]], R4),
        unify(R4,
            [believe, m3,
                m3:[
                    believe, m1, y
                ]
            ]),
    query([bout, m3, [gout, m3, [bout, m1, [mp, [bin, m1, a0], [bin, m1, a1]]]]],
        [believe, m3, [believe, m3:m1, m3:y]]
        )
    .
