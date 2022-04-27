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

#!/ usr / bin / perl - w

use strict;
use warnings;
my $file = 'theme/default/mythapps-ui.xml';

open(FH, '>', "theme/default/mythapps-ui.720.xml") or die $ !;
open my $info, $file or die "Could not open $file: $!";
my $noAlpha720 = 0;
my $noAlpha1080 = 0;
my $mcw = 0;
my $HD = 0;
my $steppes = 0;
generate();

open(FH, '>', "theme/default/mythapps-ui.Steppes.xml") or die $ !;
open $info, $file or die "Could not open $file: $!";
$noAlpha720 = 1;
$noAlpha1080 = 1;
$mcw = 0;
$HD = 1;
$steppes = 1;
generate();

open(FH, '>', "theme/default/mythapps-ui.720.NoAlpha.xml") or die $ !;
open $info, $file or die "Could not open $file: $!";
$noAlpha720 = 1;
$noAlpha1080 = 0;
$mcw = 0;
$HD = 0;
$steppes = 0;
generate();

open(FH, '>', "theme/default/mythapps-ui.720.MCW.xml") or die $ !;
open $info, $file or die "Could not open $file: $!";
$noAlpha720 = 1;
$noAlpha1080 = 0;
$mcw = 1;
$HD = 0;
$steppes = 0;
generate();

sub generate {
    my $ignoreLine = 0;
    while (my $line = <$info>) {

        if (index($line, "<!--MCW") != -1 and $mcw == 1) {
            my @spl = split('~', $line, 3);
            print FH $spl[1]."\n";
        }
        elsif((index($line, "<!--steppes") != -1) and $steppes == 1) {
            my @spl = split('_', $line, 3);
            print FH $spl[1]."\n";
        }
        elsif((index($line, "<!--720") != -1) and $HD == 0) {
            my @spl = split('!!', $line, 3);
            print FH $spl[1]."\n";
        }
        elsif((index($line, "<!--NoAlpha1080") != -1) and $noAlpha1080 == 1) {
            my @spl = split('_', $line, 3);
            print FH $spl[1]."\n";
        }
        elsif((index($line, "<!--NoAlpha720") != -1) and $noAlpha720 == 1) {
            my @spl = split('!!', $line, 3);
            print FH $spl[1]."\n";
        }
        else {
            print FH $line;
        }
    }
    close(FH);
    close $info;
}

system("sudo make install");
