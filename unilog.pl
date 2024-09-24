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
    unify_helper([], [], X, Y).

% standard unification of two expressions
unify_helper([], [], Expr, Expr) :-
    !.
% empty list is a universal symbol
unify_helper(_, _, Expr, Expr) :-
    universal(Expr),
    !.
% pop from scope
unify_helper([S|XScopeRest], YScope, X, S:Y) :-
    !,
    unify_helper(XScopeRest, YScope, X, Y).
unify_helper(XScope, [S|YScopeRest], S:X, Y) :-
    !,
    unify_helper(XScope, YScopeRest, X, Y).
% element-wise unify lists
unify_helper(XScope, YScope, [XH|XT], [YH|YT]) :-
    !,
    unify_helper(XScope, YScope, XH, YH),
    unify_helper(XScope, YScope, XT, YT).
% push onto scope
unify_helper([], YScope, X, SY) :-
    nonvar(SY), % we should never just 'generate' a scope out of thin air.
    SY = S:Y,
    !,
    append(YScope, [S], NewYScope),
    unify_helper([], NewYScope, X, Y).
unify_helper(XScope, [], SX, Y) :-
    nonvar(SX), % we should never just 'generate' a scope out of thin air.
    SX = S:X,
    !,
    append(XScope, [S], NewXScope),
    unify_helper(NewXScope, [], X, Y).



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
    rscope(RScoped, RScope, Sexpr),
        % axioms will always have their bscope
        %     at the theorem level.
    bscope(BScoped, RScope, RScoped), 
    assertz((
        unilog(RScope, BScope, [GuideTag], InSexpr) :-
                % descope the axiom according to argued BScope,
            bscope(BScoped, BScope, BDescoped),
                % then attempt to unify.
            unify(BDescoped, InSexpr)
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

unilog(RScope, BScope, [bout, NextGuide], [believe, S, Internal]) :-
    unilog(RScope, [S|BScope], NextGuide, Internal).

unilog(RScope, [S|BScope], [bin, NextGuide], Internal) :-
    unilog(RScope, BScope, NextGuide, [believe, S, Internal]).

%unilog(RScope, [S|BScope], [gout, NextGuide], [believe, S, Internal]) :-
%    append(RScope, [S], NewRScope),
%    unilog(NewRScope, )

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
    \+ query([mp, [a4], [a3]], _),
    query([bout, [mp, [bin, [a4]], [bin, [a3]]]], _),
    query([bout, [mp, [a4], [a3]]], R2),
        R2 = [believe, m1, y],
    query([bout, [bout, [mp, [bin, [a7]], [bin, [mp, [a5], [a6]]]]]], R3),
        R3 = [believe, m1, [believe, m2, c]]
    .
