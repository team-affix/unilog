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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle theorem/guide declarations

decl(Tag, Expression) :-
    %%% make sure unilog tag cannot unify with a pre-existing one.
    \+ clause(unilog(Tag, _), _),
    (
        (
            Expression = axiom(Sexpr),
            decl_axiom(Tag, Sexpr),
            !
        )
        ;
        (
            Expression = guide(Sexpr),
            decl_guide(Tag, Sexpr),
            !
        )
        ;
        (
            Expression = infer(Sexpr, Guide),
            decl_infer(Tag, Sexpr, Guide),
            !
        )
    ).

decl_axiom(tag(RScope, GuideTag), Sexpr) :-
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

%decl_infer(tag(RScope, GuideTag), Sexpr, Guide) :-
%    rscope(RScoped, RScope, Sexpr),
%    bscope(BScoped, RScope, RScoped),
%    unilog(tag(RScope, GuideTag), )
%    assertz((
%        unilog(tag(RScope, GuideTag), theorem([], InSexpr)) :-
%            unify(InSexpr, BScoped)
%    )).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

query(Tag, Theorem) :-
    unilog(tag([], Tag), theorem([], Theorem)).

:- multifile unilog/2.
:- dynamic unilog/2.

unilog(tag(RScope, [mp, ImpGuide, JusGuide]), theorem(BScope, Y)) :-
    unilog(tag(RScope, ImpGuide), theorem(BScope, [if, Y, X])),
    unilog(tag(RScope, JusGuide), theorem(BScope, X)).

unilog(tag(RScope, [bleave, NextGuide]), theorem(BScope, [believe, S, Internal])) :-
    unilog(tag(RScope, NextGuide), theorem([S|BScope], Internal)).

unilog(tag(RScope, [benter, NextGuide]), theorem([S|BScope], Internal)) :-
    unilog(tag(RScope, NextGuide), theorem(BScope, [believe, S, Internal])).

unilog(tag(RScope, [dist, UnaryGuide, NextGuide]), Theorem) :-
    (
        NextGuide = [mp, G0, G1],
        !,
        unilog(tag(RScope, [mp, [dist, UnaryGuide, G0], [dist, UnaryGuide, G1]]), Theorem)
    );
    (
        % distributing unary onto terminal guide (execute unary)
        unilog(tag(RScope, [UnaryGuide, NextGuide]), Theorem)
    ).

:-
    decl(tag([], a0), axiom([awesome, jake])),
    decl(tag([], a1), axiom([if, y, x])),
    decl(tag([], a2), axiom(x)),
    decl(tag([], a3), axiom([believe, m1, x])),
    decl(tag([], a4), axiom([believe, m1, [if, y, x]]))
    .

:- 
    \+ query([mp, a4, a3], _),
    query([bleave, [mp, [benter, a4], [benter, a3]]], _),
    query([bleave, [dist, benter, [mp, a4, a3]]], R1),
        R1 = [believe, m1, y]
    .