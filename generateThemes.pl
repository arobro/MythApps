=begin 
Myth Apps Theme. Please only edit mythapps-ui.xml and then run "perl ./generateThemes.pl" after making any changes unless you want to fork the default theme.
This will generate the changes across all themes to help increase maintainbity.

The orignal theme was designed for mythbuntu 1080 theme.

Example:
If you want a change to only apply to 720 themes, copy the below overide tag.
DEFAULT CONTENT <!--720!! CONTENT HERE !!-->

If you want a change to only apply to the myth center wide theme, copy the below overide tag. 
DEFAULT CONTENT  <!--MCW!!~ CONTENT HERE ~!!--> 

If you want a change to only apply to themes that don't properly support transparency, copy the below overide tag.
DEFAULT CONTENT <!--NoAlpha720!! CONTENT HERE !!-->

If you want a change to only apply to the steppes theme, copy the below overide tag. 
DEFAULT CONTENT  <!--steppes_ CONTENT HERE _!!--> 
=cut

#!/usr/bin/perl -w
use strict;
use warnings;

my @input_files = (
    'theme/default/mythapps-ui.xml',
    'theme/default/mythapps-music-ui.xml'
);

foreach my $file (@input_files) {
    my ($base) = $file =~ m{([^/]+)\.xml$};  # e.g. mythapps-ui

    # Variant 1: 720
    process_variant($file, "${base}.720.xml",
        noAlpha720 => 0, noAlpha1080 => 0, mcw => 0, HD => 0, steppes => 0
    );

    # Variant 2: Steppes
    process_variant($file, "${base}.Steppes.xml",
        noAlpha720 => 1, noAlpha1080 => 1, mcw => 0, HD => 1, steppes => 1
    );

    # Variant 3: 720.NoAlpha
    process_variant($file, "${base}.720.NoAlpha.xml",
        noAlpha720 => 1, noAlpha1080 => 0, mcw => 0, HD => 0, steppes => 0
    );

    # Variant 4: 720.MCW
    process_variant($file, "${base}.720.MCW.xml",
        noAlpha720 => 1, noAlpha1080 => 0, mcw => 1, HD => 0, steppes => 0
    );
}

system("sudo make install");

sub process_variant {
    my ($infile, $outfile, %flags) = @_;
    open(my $info, '<', $infile) or die "Could not open $infile: $!";
    open(my $fh, '>', "theme/default/generated/$outfile") or die $!;

    while (my $line = <$info>) {
        if (index($line, "<!--MCW") != -1 && $flags{mcw}) {
            my @spl = split('~', $line, 3);
            print {$fh} $spl[1]."\n";
        }
        elsif (index($line, "<!--steppes") != -1 && $flags{steppes}) {
            my @spl = split('_', $line, 3);
            print {$fh} $spl[1]."\n";
        }
        elsif (index($line, "<!--720") != -1 && !$flags{HD}) {
            my @spl = split('!!', $line, 3);
            print {$fh} $spl[1]."\n";
        }
        elsif (index($line, "<!--NoAlpha1080") != -1 && $flags{noAlpha1080}) {
            my @spl = split('_', $line, 3);
            print {$fh} $spl[1]."\n";
        }
        elsif (index($line, "<!--NoAlpha720") != -1 && $flags{noAlpha720}) {
            my @spl = split('!!', $line, 3);
            print {$fh} $spl[1]."\n";
        }
        else {
            print {$fh} $line;
        }
    }
    close $fh;
    close $info;
}
