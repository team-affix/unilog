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
    unify(X, [believe, S, Theorem]),
    bscope(Theorem, NextBScope, Descoped),
    !.
bscope(Theorem, [], Theorem) :-
    !.

rscope(X, [S|NextRScope], Descoped) :-
    unify(X, S:RScoped),
    rscope(RScoped, NextRScope, Descoped),
    !.
rscope(Theorem, [], Theorem) :-
    !.

fully_qualify(Unqualified, RScope, BScope, Qualified) :-
    bscope(BQualified, BScope, Unqualified),
    bscope(Qualified, RScope, BQualified).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query_entry(GuideTag, Sexpr) :-
    query(tag([], GuideTag), theorem([], Sexpr)).

query(tag(RScope, GuideTag), theorem(BScope, Sexpr)) :-
    (
        unilog(tag(RScope, GuideTag), rule(theorem(BScope, Sexpr)));
        query_fact(tag(RScope, GuideTag), theorem(BScope, Sexpr));
        unilog(tag(RScope, GuideTag), guide(theorem(BScope, Sexpr)))
    ).

% example theorem
query_fact(tag(RScope, GuideTag), theorem(BScope, InSexpr)) :-
    unilog(tag(RScope, GuideTag), fact(theorem(FactSexpr))),
    rscope(RScoped, RScope, FactSexpr),
    bscope(BRScoped, RScope, RScoped),
    bscope(BRScoped, BScope, BDescoped),
    unify(BDescoped, InSexpr).
    
unilog(tag(RScope, [tenter, S, NextGuide]), rule(theorem(BScope, BScopedY))) :-
    append(BScope, [S], NewBScope),
    query(tag(RScope, NextGuide), theorem(NewBScope, Y)),
    bscope(BScopedY, [S], Y).

unilog(tag(RScope, [mp, ImpGuide, JusGuide]), rule(theorem(BScope, Y))) :-
    query(tag(RScope, ImpGuide), theorem(BScope, [if, Y, X])),
    query(tag(RScope, JusGuide), theorem(BScope, X)).

unilog(tag([], a0), fact(theorem([awesome, _]))).
unilog(tag([m1], a0), fact(theorem([awesome, _]))).

unilog(tag([], a1), fact(theorem([if, b, a]))).
unilog(tag([], a2), fact(theorem(a))).
