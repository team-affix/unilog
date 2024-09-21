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

decl(Tag, Expression) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(Tag, _), _),
    (
        (
            Expression = theorem(Sexpr),
            decl_theorem(Tag, Sexpr),
            !
        )
        ;
        (
            Expression = guide(Sexpr),
            decl_guide(Tag, Sexpr),
            !
        )
    ).

decl_theorem(tag(RScope, GuideTag), Sexpr) :-
    rscope(RScoped, RScope, Sexpr),
    bscope(BScoped, RScope, RScoped),
    assertz((
        unilog(tag(RScope, GuideTag), theorem([], InSexpr)) :-
            unify(InSexpr, BScoped)
    )).

decl_guide(tag(RScope, GuideTag), Redirect) :-
    assertz((
        unilog(tag(RScope, GuideTag), Theorem) :-
            unilog(tag(RScope, Redirect), Theorem)
    )).

query(Tag, Theorem) :-
    unilog(tag([], Tag), theorem([], Theorem)).

:- multifile unilog/2.
:- dynamic unilog/2.

unilog(tag(RScope, [mp, ImpGuide, JusGuide]), theorem(BScope, Y)) :-
    unilog(tag(RScope, ImpGuide), theorem(BScope, [if, Y, X])),
    unilog(tag(RScope, JusGuide), theorem(BScope, X)).

unilog(tag([], a0), theorem([], InSexpr)) :-
    unify(InSexpr, [awesome, _]).
unilog(tag([m1], a0), theorem([], InSexpr)) :-
    unify(InSexpr, [believe, m1, m1:[awesome, _]]).

unilog(tag([], a1), theorem([], InSexpr)) :-
    unify(InSexpr, [if, b, a]).
unilog(tag([], a2), theorem([], InSexpr)) :-
    unify(InSexpr, a).
