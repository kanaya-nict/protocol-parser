#!/usr/bin/env perl
use IO::Socket::UNIX;
use JSON;
use MIME::Base64 qw(encode_base64);

my $name;

if (@ARGV > 0) {
    $name = $ARGV[0];
} else {
    $name = "/tmp/sf-tap/udp/syslog";
}

sub parse_header{
    my ($line) = @_;
    chop $line;
    my %data = split(/[\,\=]/, $line);

    my $res;

    if($data{from} == 2 ){
        $res = {
            src=>$data{ip2},
            dst=>$data{ip1},
            len=>$data{len}
        };
    } else {
        $res = {
            src=>$data{ip1},
            dst=>$data{ip2},
            len=>$data{len}
        };
    };
    
    return $res;
}


sub parse_body{
    my ($line) = @_;
    my $org_line = encode_base64($line);
    $line=~ s/^\<(\d+)\>(\w+ \w+ \w+\:\w+\:\w+) ([^\s]+)\s+([a-zA-Z]+)//;
    my $res = {
        pri =>$1,
        date=>$2,
        host=>$3,
        tag =>$4,
        row =>$org_line
    };
    return $res;
}

my $s = IO::Socket::UNIX->new(Type=>SOCK_STREAM(), Peer=>$name);


my $line;

while(defined ($line = $s->getline())){
    my $header;
    my %data;
    if(defined ($header = parse_header($line))){
        $data{header} = $header;
        my $res; my $body;
        $res = $s->read($body, $header->{len});
        $data{body} = parse_body($body);
        print encode_json(\%data), "\n";
    }
}


