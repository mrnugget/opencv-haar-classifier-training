#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cvhaartraining.h>
#include <_cvhaartraining.h> // Load CvVecFile
// Write a vec header into the vec file (located at cvsamples.cpp)
void icvWriteVecHeader( FILE* file, int count, int width, int height );
// Write a sample image into file in the vec format (located at cvsamples.cpp)
void icvWriteVecSample( FILE* file, CvArr* sample );
// Append the body of the input vec to the ouput vec
void icvAppendVec( CvVecFile &in, CvVecFile &out, int *showsamples, int winwidth, int winheight );
// Merge vec files
void icvMergeVecs( char* infoname, const char* outvecname, int showsamples, int width, int height );

// Append the body of the input vec to the ouput vec
void icvAppendVec( CvVecFile &in, CvVecFile &out, int *showsamples, int winwidth, int winheight )
{
    CvMat* sample;

    if( *showsamples )
    {
        cvNamedWindow( "Sample", CV_WINDOW_AUTOSIZE );
    }
    if( !feof( in.input ) )
    {
        in.last = 0;
        in.vector = (short*) cvAlloc( sizeof( *in.vector ) * in.vecsize );
        if ( *showsamples )
        {
            if ( in.vecsize != winheight * winwidth )
            {
                fprintf( stderr, "ERROR: -show: the size of images inside of vec files does not match with %d x %d, but %d\n", winheight, winwidth, in.vecsize );
                exit(1);
            }
            sample = cvCreateMat( winheight, winwidth, CV_8UC1 );
        } 
        else 
        {
            sample = cvCreateMat( in.vecsize, 1, CV_8UC1 );
        }
        for( int i = 0; i < in.count; i++ )
        {
            icvGetHaarTraininDataFromVecCallback( sample, &in );
            icvWriteVecSample ( out.input, sample );
            if( *showsamples )
            {
                cvShowImage( "Sample", sample );
                if( cvWaitKey( 0 ) == 27 )
                { 
                    *showsamples = 0; 
                }
            }
        }
        cvReleaseMat( &sample );
        cvFree( (void**) &in.vector );
    }
}

void icvMergeVecs( char* infoname, const char* outvecname, int showsamples, int width, int height )
{
    char onevecname[PATH_MAX];
    int i = 0;
    int filenum = 0;
    short tmp; 
    FILE *info;
    CvVecFile outvec;
    CvVecFile invec;
    int prev_vecsize;

    // fopen input and output file
    info = fopen( infoname, "r" );
    if ( info == NULL )
    {
        fprintf( stderr, "ERROR: Input file %s does not exist or not readable.\n", infoname );
        exit(1);
    }
    outvec.input = fopen( outvecname, "wb" );
    if ( outvec.input == NULL )
    {
        fprintf( stderr, "ERROR: Output file %s is not writable.\n", outvecname );
        exit(1);
    }

    // Header
    rewind( info );
    outvec.count = 0;
    for ( filenum = 0; ; filenum++ )
    {
        if ( fscanf( info, "%s", onevecname ) == EOF )
        {
            break;
        }
        invec.input = fopen( onevecname, "rb" );
        if ( invec.input == NULL )
        {
            fprintf( stderr, "ERROR: Input file %s does not exist or not readable.\n", onevecname );
            exit(1);
        }
        fread( &invec.count,   sizeof( invec.count )  , 1, invec.input );
        fread( &invec.vecsize, sizeof( invec.vecsize ), 1, invec.input );
        fread( &tmp, sizeof( tmp ), 1, invec.input );
        fread( &tmp, sizeof( tmp ), 1, invec.input );

        outvec.count += invec.count;
        if( i > 0 &&  invec.vecsize != prev_vecsize )
        {
            fprintf( stderr, "ERROR: The size of images in %s(%d) is different with the previous vec file(%d).\n", onevecname, invec.vecsize, prev_vecsize );
            exit(1);
        }
        prev_vecsize = invec.vecsize;
        fclose( invec.input );
    }
    outvec.vecsize = invec.vecsize;
    icvWriteVecHeader( outvec.input, outvec.count, outvec.vecsize, 1);

    // Contents
    rewind( info );
    outvec.count = 0;
    for ( i = 0; i < filenum ; i++ )
    {
        if (fscanf( info, "%s", onevecname ) == EOF) {
            break;
        }
        invec.input = fopen( onevecname, "rb" );
        fread( &invec.count,   sizeof( invec.count )  , 1, invec.input );
        fread( &invec.vecsize, sizeof( invec.vecsize ), 1, invec.input );
        fread( &tmp, sizeof( tmp ), 1, invec.input );
        fread( &tmp, sizeof( tmp ), 1, invec.input );

        icvAppendVec( invec, outvec, &showsamples, width, height );
        fclose( invec.input );
    }
    fclose( outvec.input );
}

int main( int argc, char **argv ) 
{
    int i;
    char *infoname   = NULL;
    char *outvecname = NULL;
    int showsamples  = 0;
    int width        = 24;
    int height       = 24;

    if( argc == 1 )
    {
        printf( "Usage: %s\n  <collection_file_of_vecs>\n"
            "  <output_vec_filename>\n"
            "  [-show] [-w <sample_width = %d>] [-h <sample_height = %d>]\n",
            argv[0], width, height );
        return 0;
    }
    for( i = 1; i < argc; ++i )
    {
        if( !strcmp( argv[i], "-show" ) )
        {
            showsamples = 1;
            // width = atoi( argv[++i] ); // obsolete -show width height
            // height = atoi( argv[++i] );
        } 
        else if( !strcmp( argv[i], "-w" ) )
        {
            width = atoi( argv[++i] );
        } 
        else if( !strcmp( argv[i], "-h" ) )
        {
            height = atoi( argv[++i] );
        }
        else if( argv[i][0] == '-' )
        {
            fprintf( stderr, "ERROR: The option %s does not exist. n", argv[i] );
            exit(1);
        }
        else if( infoname == NULL )
        {
            infoname = argv[i];
        }
        else if( outvecname == NULL )
        {
            outvecname = argv[i];
        }
    }
    if( infoname == NULL )
    {
        fprintf( stderr, "ERROR: No input file\n" );
        exit(1);
    }
    if( outvecname == NULL )
    {
        fprintf( stderr, "ERROR: No output file\n" );
        exit(1);
    }
    icvMergeVecs( infoname, outvecname, showsamples, width, height );
    return 0;
}
