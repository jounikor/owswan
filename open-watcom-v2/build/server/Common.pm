############################################################################
#
#                            Open Watcom Project
#
#  Copyright (c) 2004-2006 The Open Watcom Contributors. All Rights Reserved.
#
#  ========================================================================
#
#    This file contains Original Code and/or Modifications of Original
#    Code as defined in and that are subject to the Sybase Open Watcom
#    Public License version 1.0 (the 'License'). You may not use this file
#    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
#    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
#    provided with the Original Code and Modifications, and is also
#    available at www.sybase.com/developer/opensource.
#
#    The Original Code and all software distributed under the License are
#    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
#    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
#    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
#    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
#    NON-INFRINGEMENT. Please see the License for the specific language
#    governing rights and limitations under the License.
#
#  ========================================================================
#
#  Description: Common definitions for the various scripts.
#
###########################################################################
use strict;

package Common;

my($OWloc) = '';

$\ = "\n";

sub read_config
{
    my($filename) = $_[0];
    my(@fields);

    $Common::config{'OWCVS'} = '';
    $Common::config{'OWCVSARGS'} = '';
    $Common::config{'TOOLS'} = '';
    $Common::config{'RELROOT'} = '';
    $Common::config{'WIN95HC'} = '';
    $Common::config{'HHC'} = '';
    $Common::config{'GHOSTSCRIPTPATH'} = '';
    $Common::config{'DOSBOX'} = '';
    $Common::config{'INITCMD'} = '';
    open(CONFIG_FILE, $filename) || die "Unable to open configuration file: $filename.";
    while (<CONFIG_FILE>) {
        s/\r?\n/\n/;
        chomp;
        s/#.*//;
        if (/^\s*$/) { next; }
        @fields = split /=/, $_, 2;
        $Common::config{$fields[0]} = $fields[1];
    }
    close(CONFIG_FILE);
    if (defined($Common::config{'OW'})) {
        $OWloc = $Common::config{'OW'};
        $OWloc =~ s/\\/\\\\/g;
    }
}

sub remove_OWloc
{
    my($txt) = $_[0];

    if ($^O eq 'linux') {
        $txt =~ s/$OWloc[\/\\]//g;
    } else {
        $txt =~ s/$OWloc[\/\\]//gi;
    }
    return $txt;
}

sub process_summary
{
    my($inp_filename)    = $_[0];
    my($out_filename)    = $_[1];
    my($current_project) = '';
    my(@header);

    if( not -e $inp_filename ) {
        # Create non-existing input file to be able proccess for first time.
        open(INFILE, '>', $inp_filename) || die "Unable to open input file: $inp_filename";
        close(INFILE);
    }
    open(INFILE, $inp_filename) || die "Unable to open input file: $inp_filename";
    open(OUTFILE, '>', $out_filename) || die "Unable to open output file: $out_filename";

    # Read the build log file a line at a time and output the error summary.
    while (<INFILE>) {
        s/\r?\n/\n/;
        chomp;
        if (/^[=]+ .* [=]+$/) {
            @header = split;
            $current_project = remove_OWloc($header[2]);
        } elsif (/Error!|\*ERROR\*| error |Warning!|\*WARNING\*| WARNING: |Can not/) {
            print OUTFILE '';
            print OUTFILE "PROJECT $current_project";
            print OUTFILE remove_OWloc($_);
        }
    }
    close(OUTFILE);
    close(INFILE);
}

# Read the next two line record from the summary file.
sub read_record
{
    my($file) = $_[0];
    my($record, $line);
    while (<$file>) {
        s/\r?\n/\n/;
        chomp;
        s/#.*//;
        if (/^\s*$/) { next; }
        $record = $_;
        $line = <$file>;
        chomp($line);
        $record = $record . '|' . $line;
        return $record;
    }
    return 'EOF';
}

# Display a combined record as two lines.
sub print_record
{
    my($record) = $_[0];
    my($fh) = $_[1];

    my(@fields) = split /\|/, $record;
    print $fh "$fields[0]";
    print $fh "$fields[1]";
    print $fh '';
}

sub process_compare
{
    my($filename1) = $_[0];
    my($filename2) = $_[1];
    my($fh) = $_[2];
    my($record, @old_records, @new_records, $found, $candidate);
    my($something_added, $something_removed, $first_added);
    my($first_removed, $exit_status);

    $fh ||= \*STDOUT;

    # Read both the old and new summaries into memory.
    if( -e $filename1 ) {
        open(OLDFILE, $filename1) || die "Unable to open input file: $filename1";
        while (($record = read_record(\*OLDFILE)) ne 'EOF') {
            push @old_records, $record;
        }
        close(OLDFILE);
    }

    if( -e $filename2 ) {
        open(NEWFILE, $filename2) || die "Unable to open output file: $filename2";
        while (($record = read_record(\*NEWFILE)) ne 'EOF') {
            push @new_records, $record;
        }
        close(NEWFILE);
    }

    # Now compare the summaries. This runs in O(n^2) where n is the # of records.
    #############################################################################

    $something_added   = 'no';
    $something_removed = 'no';
    $first_added       = 'yes';
    $first_removed     = 'yes';

    $something_added = 'no';
    foreach $record (@new_records) {
        $found = 'no';
        if (@old_records) {
            foreach $candidate (@old_records) {
                if ($record eq $candidate) { $found = 'yes'; }
            }
        }
        if ($found eq 'no') {
            if ($first_added eq 'yes') {
                print $fh 'Messages Added';
                print $fh '--------------';
                print $fh '';
                $first_added = 'no';
            }   
            print_record($record, $fh);
            $something_added = 'yes';
        }
    }

    # If there are new errors, don't bother computing removed messages. Some
    # messages might appear to vanish because certain compilations failed to
    # finish. Only trust the removal list if there are no additional errors.
    #
    if ($something_added eq 'no' and @old_records) {
        foreach $record (@old_records) {
            $found = 'no';
            foreach $candidate (@new_records) {
                if ($record eq $candidate) { $found = 'yes'; }
            }
            if ($found eq 'no') {
                if ($first_removed eq 'yes') {
                    print $fh 'Messages Removed';
                    print $fh '----------------';
                    print $fh '';
                    $first_removed = 'no';
                }   
                print_record($record, $fh);
                $something_removed = 'yes';
            }
        }
    }

    $exit_status = 1;  # Assume failure.

    # This is what we like to see.
    if ($something_added eq 'no' && $something_removed eq 'no') {
        print $fh 'Build Successful';
        print $fh '';
        $exit_status = 0;
    }
    return $exit_status;
}

1;
