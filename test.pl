prefix_args(_, [], []).
prefix_args(Prefix, [A|Rest], [A2|ConvertedRest]) :-
    A = Prefix:A2,
    prefix_args(Prefix, Rest, ConvertedRest).