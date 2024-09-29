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

% standard unification of two expressions
punify(Expr, Expr) :-
    !.
% empty list is a universal symbol
punify(Expr, _:Expr) :-
    universal(Expr),
    !.
punify(_:Expr, Expr) :-
    universal(Expr),
    !.
punify(_:Expr, _:Expr) :-
    universal(Expr),
    !.
punify(X, S:[YH|YT]) :-
    !,
    punify(X, [S:YH|S:YT]).
punify(S:[XH|XT], Y) :-
    !,
    punify([S:XH|S:XT], Y).
punify([XH|XT], [YH|YT]) :-
    !,
    punify(XH, YH),
    punify(XT, YT).

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

rscope(X, [S|NextRScope], Descoped) :-
    unify(X, S:RScoped),
    rscope(RScoped, NextRScope, Descoped),
    !.
rscope(SExpr, [], SExpr) :-
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

axiom(RScope, GuideTag, Sexpr) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(RScope, _, GuideTag, _), _),
    atomic(GuideTag),
    assertz((
        unilog(RScope, BScope, GuideTag, InSexpr) :-
                % descope the axiom according to argued BScope,
            bscope(BScoped, BScope, InSexpr),
                % then attempt to unify.
            unify(BScoped, Sexpr)
    )).

guide(RScope, GuideTag, GuideArgs, Redirect) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(RScope, _, GuideTag, _), _),
    atomic(GuideTag),
    is_list(GuideArgs),
    assertz((
        unilog(RScope, BScope, [GuideTag|GuideArgs], SExpr) :-
            unilog(RScope, BScope, Redirect,  SExpr)
    )).

%infer(RScope, GuideTag, Sexpr, Guide) :-
%    %%% make sure unilog tag cannot unify with a pre-existing one.
%    \+ clause(unilog(RScope, _, GuideTag, _), _),
%    rscope(RScoped, RScope, Sexpr),
%    bscope(BScoped, RScope, RScoped),
%    query()
%    assertz((
%        unilog(RScope, [], GuideTag, InSexpr) :-
%            unify(InSexpr, BScoped)
%    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(Tag, SExpr) :-
    unilog([], [], Tag, SExpr).

:- multifile unilog/4.
:- dynamic unilog/4.

unilog(RScope, BScope, [bout, S, NextGuide], [believe, S, Internal]) :-
    unilog(RScope, [S|BScope], NextGuide, Internal).

unilog(RScope, [S|BScope], [bin, S, NextGuide], Internal) :-
    unilog(RScope, BScope, NextGuide, [believe, S, Internal]).

unilog(RScope, [S|BScope], [gout, S, NextGuide], RScoped) :-
    append(RScope, [S], NewRScope),
    rscope(RScoped, [S], RDescoped),
    unilog(NewRScope, BScope, NextGuide, RDescoped).

% logic ROI

unilog(RScope, BScope, [mp, ImpGuide, JusGuide], Y) :-
    unilog(RScope, BScope, ImpGuide, [if, Y, X]),
    unilog(RScope, BScope, JusGuide, X).

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
    query([bout, m1, [mp, a4, a3]], R2),
        R2 = [believe, m1, y],
    query([bout, m1, [bout, m2, [mp, [bin, m2, a7], [bin, m2, [mp, a5, a6]]]]], R3),
        R3 = [believe, m1, [believe, m2, c]],
    query([bout, m3, [gout, m3, [bout, m1, [mp, a0, a1]]]], R4),
        R4 = [believe, m3,
            m3:[
                believe, m1, y
            ]
        ],
    query([bout, m3, [gout, m3, [bout, m1, [mp, a0, a1]]]],
        [believe, m3, [believe, m3:m1, m3:y]]
        ),
    \+ query([bout, m3, [gout, m3, [bout, m1, [mp, a0, a1]]]],
        [believe, m4, [believe, m3:m1, m3:y]]
        ),
    \+ query([bout, m3, [gout, m3, [bout, m1, [mp, a0, a1]]]],
        [believe, m3, [believe, m1, m3:y]]
        ),
    \+ query([bout, m3, [gout, m3, [bout, m1, [mp, a0, a1]]]],
        [believe, m3, [believe, m3:m1, y]]
        )
    .
