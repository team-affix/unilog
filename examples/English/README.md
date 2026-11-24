# English, translated

Real English sentences written as Unilog, each proved well-formed, and a few beliefs that lead from them to a
view: "Jake was kind to Walter".

| File | What it is |
|---|---|
| `Grammar.ul` | Which sentences are well-formed: `wf S`, from what kind each word is and the sort of each name |
| `Words.ul` | The dictionary: each word's kind (`noun dog`, `verb run`, `role agent`, ...) and what words mean in terms of each other ("every dog is an animal", "driving someone somewhere helps them") |
| `Sentences.ul` | Sixteen sentences, each broken into facts, every fact with its `_wf` proof |
| `Views.ul` | Two opinions, and what follows from them and the sentences |

## The method

Name what the sentence is about, then state each piece of meaning as one small fact.

| English | Becomes | Example |
|---|---|---|
| A specific thing or person ("Jake", "the dog") | a name | `jake`, `the_dog` |
| A noun ("is a dog") | a fact about the thing | `dog the_dog` |
| An adjective ("is steep") | a fact about the thing | `steep the_hill` |
| A graded adjective ("a tall child") | measured against a kind | `tall_for child tom` |
| A verb ("ran") | a named event, and its kind | `run jakes_run` |
| Who or what takes part ("Jake ran", "drove his grandfather", "to the store") | a role of the event | `agent jakes_run jake`, `theme jakes_drive jakes_grandfather`, `goal jakes_drive the_store` |
| An adverb ("carefully", "up") | a fact about the event | `careful jakes_drive`, `upward the_dogs_run` |
| A state ("was tired") | a named event, with a holder | `tired jakes_tiredness`, `holder jakes_tiredness jake` |
| Tense ("ran", "will visit") | a date | `on jakes_run day_2026_11_23` |
| Progressive ("while Jake was driving") | one event happens during another | `starts_during the_rain jakes_drive` |
| "because" | one event causes another | `causes the_rain the_wetness` |
| "is" as identity ("is Walter") | equality | `eq jakes_grandfather walter` |
| "believes that ..." | a stance toward a whole sentence | `believes jake (open the_store)` |
| "every" | for every X, if ... then ... | `\X. dog X => animal X` |
| "some", "a" | there is one | `exists \D. ...` |
| "never", "not" | ... implies false | `\E. bark E => agent E the_dog => target E the_stranger => false` |

**Why events?** "Jake drove his grandfather to the store" is not `drove jake grandfather store`. It is one
driving with three facts about it. So "Jake drove his grandfather" follows by leaving a fact out, and adding
"carefully" or "yesterday" adds a fact without changing any word.

**One symbol per meaning, in its plain form.** `run`, never `ran` or `running`: tense is a date, not a word
form. Dates are absolute, never "now", because a published package never changes.

## Well-formedness

`wf S` says sentence S is well-formed. It is proved, like anything else, from `Grammar.ul`: a word's kind says
what it applies to (a verb to an event, a role to an event and a thing), and each name has a sort (`entity jake`,
`event jakes_run`, `time day_2026_11_23`). So `wf (run jakes_run)` is provable, and `wf (run the_dog)` is not:
"the dog is a running" mixes up a thing and an event. Unilog says so:

```
Cannot apply wf_verb run_verb to the_dog_entity.
  wf_verb run_verb proves: event the_dog => wf (run the_dog)
  the_dog_entity proves: entity the_dog
  event does not match entity
```

Nothing stops anyone from writing any symbols they like. A sentence without a `wf` proof is simply not shown
to be well-formed, and a reader (or a server) can ask for one.

## Things to try

From `examples/`:

```bash
unilog rests-on English.Views.jake_was_kind_to_walter     # the facts, word meanings and opinions it needs
unilog proof English.Views.jake_was_kind_to_his_grandfather --shallow
unilog find "kind_to jake jakes_grandfather"                  # two different arguments prove it
unilog find "\E. run E => exercise E"                         # a word meaning, found by its statement
unilog axioms English.Views                                   # the two opinions: everything else comes from the other files
```
