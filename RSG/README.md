### Random Sentence Generation (RSG)

Generate random sentence according to given grammer.

### Example

Given grammer 

```
The Poem grammar
{
<start>
The <object> <verb> tonight. ;
}
{
<object>
waves ;
big yellow flowers ;
slugs ;
}
{
<verb>
sigh <adverb> ;
portend like <object> ;
die <adverb> ;
}
{
<adverb>
warily ;
grumpily ;
}

```

Results could be

```
The waves die warily tonight.
The waves portend like big yellow flowers tonight.
The slugs portend like big yellow flowers tonight.
```