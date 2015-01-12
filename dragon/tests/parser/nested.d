program example(input, output);
var x, y: integer;
function gcd(a, b: integer): integer;
    function md(a, b: integer): integer;
    begin
        md := a mod b;
        foo
    end;
begin
    if b = 0 then gcd := a
    else gcd := gcd(b, md(a, b))
end;

begin
    read(x, y);
    write(gcd(x, y))
end.
