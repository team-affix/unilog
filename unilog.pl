% ROI listed here

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Helper

without_last([_], []) :- !.
without_last([X|Rest], [X|RestWithoutLast]) :-
    without_last(Rest, RestWithoutLast),
    !.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle Scoping

% Recursive case: split the left and right side of the colon and convert each part to a list.
resolve(H:T, [H|Rest], SExpr) :-
    resolve(T, Rest, SExpr).

% Base case: a single expression belonging to the root scope.
resolve(SExpr, [], SExpr).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Handle querying

unilog(tag(RScope, [mp, ImpGuide, JusGuide]), theorem(SScope, Symbol)) :-
    unilog(tag(RScope, ImpGuide), theorem(SScope, ImpSExpr)),
    unilog(tag(RScope, JusGuide), theorem(SScope, ImpSExpr)),
    
