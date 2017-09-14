% Special submit rule for changes created by Findit.
% If Findit wants to commit a change it created,
% the change is submittable only if
% 1. The change is on master branch,
% 2. The change is a pure revert of another change.
submit_rule(S) :-
    gerrit:change_owner(user(1193694)),
    change_is_on_master([], M),
    commit_is_pure_revert(M, PR),
    S =.. [submit | PR].

% For changes that are NOT created by Findit, they should follow default submit
% rule.
submit_rule(S) :-
  \+ gerrit:change_owner(user(1193694)),
  gerrit:default_submit(S).

% The change is on master branch.
change_is_on_master(S, M) :-
    gerrit:change_branch('refs/heads/master'),
    !,
    gerrit:change_owner(A),
    M = [label('Change-On-Master', ok(A)) | S].

change_is_on_master(S, M) :-
    gerrit:change_branch(B),
    B \= 'refs/heads/master',
    gerrit:change_owner(A),
    M = [label('Change-On-Master', reject(A)) | S].

% This commit is pure revert of another commit.
commit_is_pure_revert(S, PR) :-
    gerrit:pure_revert(1),
    !,
    gerrit:change_owner(A),
    PR = [label('Is-Pure-Revert', ok(A)) | S].

commit_is_pure_revert(S, PR) :-
    gerrit:pure_revert(U),
    U \= 1,
    gerrit:change_owner(A),
    PR = [label('Is-Pure-Revert', reject(A)) | S].