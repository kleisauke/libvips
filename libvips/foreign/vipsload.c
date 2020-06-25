/* load vips from a file
 *
 * 24/11/11
 *
 * 25/6/20 kleisauke
 * 	- rewrite for source API
 */

/*

    This file is part of VIPS.
    
    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>

#include <vips/vips.h>
#include <vips/internal.h>

typedef struct _VipsForeignLoadVips {
	VipsForeignLoad parent_object;

	/* Set by subclasses.
	 */
	VipsSource *source;

} VipsForeignLoadVips;

typedef VipsForeignLoadClass VipsForeignLoadVipsClass;

G_DEFINE_ABSTRACT_TYPE( VipsForeignLoadVips, vips_foreign_load_vips, 
	VIPS_TYPE_FOREIGN_LOAD );

static void
vips_foreign_load_vips_dispose( GObject *gobject )
{
	VipsForeignLoadVips *vips = (VipsForeignLoadVips *) gobject;

	VIPS_UNREF( vips->source );

	G_OBJECT_CLASS( vips_foreign_load_vips_parent_class )->
		dispose( gobject );
}

static VipsForeignFlags
vips_foreign_load_vips_get_flags_source( VipsSource *source  )
{
	static unsigned char sig_sparc[4] = { 182, 166, 242, 8 };

	VipsForeignFlags flags;

	flags = VIPS_FOREIGN_PARTIAL;

	const unsigned char *p;

	if( (p = vips_source_sniff( source, 4 )) &&
		memcmp( p, sig_sparc, 4 ) == 0 )
		flags |= VIPS_FOREIGN_BIGENDIAN;

	return( flags );
}

static VipsForeignFlags
vips_foreign_load_vips_get_flags( VipsForeignLoad *load )
{
	VipsForeignLoadVips *vips = (VipsForeignLoadVips *) load;

	return( vips_foreign_load_vips_get_flags_source( vips->source ) );
}

static VipsForeignFlags
vips_foreign_load_vips_file_get_flags_filename( const char *filename )
{
	VipsForeignFlags flags;

	flags = VIPS_FOREIGN_PARTIAL;

	if( vips__file_magic( filename ) == VIPS_MAGIC_SPARC )
		flags |= VIPS_FOREIGN_BIGENDIAN;

	return( flags );
}

static int
vips_foreign_load_vips_header( VipsForeignLoad *load )
{
	VipsForeignLoadVips *vips = (VipsForeignLoadVips *) load;
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( vips );

	VipsImage *image;
	VipsImage *x;

	unsigned char header[VIPS_SIZEOF_HEADER];
	gint64 psize;
	gint64 rsize;

	/* We use "p" mode to get it opened as a partial vips image, 
	 * bypassing the file type checks.
	 */
	if( !(image = vips_image_new_mode( vips_connection_filename( 
		VIPS_CONNECTION( vips->source ) ), "p" )) )
		return( -1 );

	if( vips_source_rewind( vips->source ) )
		return( -1 );

	if( vips_source_read( vips->source, header, VIPS_SIZEOF_HEADER ) !=
		VIPS_SIZEOF_HEADER ||
		vips__read_header_bytes( image, header ) ) {
		vips_error( class->nickname,
			"%s", _( "unable to read header" ) );
		return( -1 );
	}

	/* Predict and check the file size. Only issue a warning, we want to be
	 * able to read all the header fields we can, even if the actual data
	 * isn't there. 
	 */
	psize = vips__image_pixel_length( image );
	if( (rsize = vips_source_length( vips->source )) == -1 )
		return( -1 );
	image->file_length = rsize;
	if( psize > rsize )
		g_warning( _( "unable to read data for \"%s\", %s" ),
			vips_connection_nick( VIPS_CONNECTION( vips->source ) ),
			_( "file has been truncated" ) );

	/* Set demand style. This suits a disc file we read sequentially.
	 */
	image->dhint = VIPS_DEMAND_STYLE_THINSTRIP;

	/* Set the history part of im descriptor. Don't return an error if this
	 * fails (due to eg. corrupted XML) because it's probably mostly
	 * harmless.
	 */
	if( vips__readhist_source( vips->source, image ) ) {
		g_warning( _( "error reading vips image metadata: %s" ),
			vips_error_buffer() );
		vips_error_clear();
	}

	VIPS_SETSTR( image->filename,
		vips_connection_filename( VIPS_CONNECTION( vips->source ) ) );

	/* mmap() the whole thing.
	 */
	if( !(image->baseaddr = (void *) vips_source_map( 
		vips->source, &image->file_length )) )
		return( -1 );

	image->dtype = VIPS_IMAGE_MMAPIN;
	image->data = (VipsPel *) image->baseaddr + image->sizeof_header;

	vips_source_minimise( vips->source );

	/* Remove the @out that's there now. 
	 */
	g_object_get( load, "out", &x, NULL );
	g_object_unref( x );
	g_object_unref( x );

	g_object_set( load, "out", image, NULL );

	return( 0 );
}

static void
vips_foreign_load_vips_class_init( VipsForeignLoadVipsClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->dispose = vips_foreign_load_vips_dispose;

	object_class->nickname = "vipsload_base";
	object_class->description = _( "load vips base class" );

	/* We are fast at is_a(), so high priority.
	 */
	foreign_class->priority = 200;

	load_class->get_flags = vips_foreign_load_vips_get_flags;
	load_class->get_flags_filename = 
		vips_foreign_load_vips_file_get_flags_filename;
	load_class->header = vips_foreign_load_vips_header;
	load_class->load = NULL;

}

static void
vips_foreign_load_vips_init( VipsForeignLoadVips *vips )
{
}

typedef struct _VipsForeignLoadVipsFile {
	VipsForeignLoadVips parent_object;

	/* Filename for load.
	 */
	char *filename;

} VipsForeignLoadVipsFile;

typedef VipsForeignLoadVipsClass VipsForeignLoadVipsFileClass;

G_DEFINE_TYPE( VipsForeignLoadVipsFile, vips_foreign_load_vips_file,
		vips_foreign_load_vips_get_type() );

static gboolean
vips_foreign_load_vips_file_is_a( const char *filename )
{
	return( vips__file_magic( filename ) );
}

static int
vips_foreign_load_vips_file_build( VipsObject *object )
{
	VipsForeignLoadVips *vips = (VipsForeignLoadVips *) object;
	VipsForeignLoadVipsFile *file = (VipsForeignLoadVipsFile *) object;

	if( file->filename &&
		!(vips->source = vips_source_new_from_file( file->filename )) )
			return( -1 );

	if( VIPS_OBJECT_CLASS( vips_foreign_load_vips_file_parent_class )->
		build( object ) )
		return( -1 );

	return( 0 );
}

const char *vips__suffs[] = { ".v", ".vips", NULL };

static void
vips_foreign_load_vips_file_class_init( 
	VipsForeignLoadVipsFileClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "vipsload";
	object_class->description = _( "load vips from file" );
	object_class->build = vips_foreign_load_vips_file_build;

	foreign_class->suffs = vips__suffs;

	load_class->is_a = vips_foreign_load_vips_file_is_a;

	VIPS_ARG_STRING( class, "filename", 1, 
		_( "Filename" ),
		_( "Filename to load from" ),
		VIPS_ARGUMENT_REQUIRED_INPUT, 
		G_STRUCT_OFFSET( VipsForeignLoadVipsFile, filename ),
		NULL );
}

static void
vips_foreign_load_vips_file_init( VipsForeignLoadVipsFile *vips )
{
}

typedef struct _VipsForeignLoadVipsSource {
	VipsForeignLoadVips parent_object;

	VipsSource *source;

} VipsForeignLoadVipsSource;

typedef VipsForeignLoadVipsClass VipsForeignLoadVipsSourceClass;

G_DEFINE_TYPE( VipsForeignLoadVipsSource, vips_foreign_load_vips_source,
	vips_foreign_load_vips_get_type() );

static int
vips_foreign_load_vips_source_build( VipsObject *object )
{
	VipsForeignLoadVips *vips = (VipsForeignLoadVips *) object;
	VipsForeignLoadVipsSource *source = (VipsForeignLoadVipsSource *) object;

	if( source->source ) {
		vips->source = source->source;
		g_object_ref( vips->source );
	}

	if( VIPS_OBJECT_CLASS( vips_foreign_load_vips_source_parent_class )->
		build( object ) )
		return( -1 );

	return( 0 );
}

static int
vips_foreign_load_vips_source_is_a_source( VipsSource *source )
{
	static unsigned char sig_intel[4] = { 8, 242, 166, 182 };
	static unsigned char sig_sparc[4] = { 182, 166, 242, 8 };

	const unsigned char *p;

	if( (p = vips_source_sniff( source, 4 )) &&
		(memcmp( p, sig_intel, 4 ) == 0 ||
		 memcmp( p, sig_sparc, 4 ) == 0) )
		return( TRUE );

	return( FALSE );
}

static void
vips_foreign_load_vips_source_class_init( 
	VipsForeignLoadVipsSourceClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignLoadClass *load_class = (VipsForeignLoadClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "vipsload_source";
	object_class->description = _( "load vips from source" );
	object_class->build = vips_foreign_load_vips_source_build;

	load_class->is_a_source = vips_foreign_load_vips_source_is_a_source;

	VIPS_ARG_OBJECT( class, "source", 1,
		_( "Source" ),
		_( "Source to load from" ),
		VIPS_ARGUMENT_REQUIRED_INPUT, 
		G_STRUCT_OFFSET( VipsForeignLoadVipsSource, source ),
		VIPS_TYPE_SOURCE );

}

static void
vips_foreign_load_vips_source_init( VipsForeignLoadVipsSource *source )
{
}

/**
 * vips_vipsload:
 * @filename: file to load
 * @out: (out): decompressed image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Read in a vips image. 
 *
 * See also: vips_vipssave().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_vipsload( const char *filename, VipsImage **out, ... )
{
	va_list ap;
	int result;

	va_start( ap, out );
	result = vips_call_split( "vipsload", ap, filename, out );
	va_end( ap );

	return( result );
}

/**
 * vips_vipsload_source:
 * @source: source to load
 * @out: (out): output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Exactly as vips_vipsload(), but read from a source. 
 *
 * See also: vips_vipsload().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_vipsload_source( VipsSource *source, VipsImage **out, ... )
{
	va_list ap;
	int result;

	va_start( ap, out );
	result = vips_call_split( "vipsload_source", ap, source, out );
	va_end( ap );

	return( result );
}
