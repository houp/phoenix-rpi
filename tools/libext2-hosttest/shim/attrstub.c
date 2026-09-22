/* Host shim: libphoenix's _phoenix_initAttrsStruct(), used only by
 * ext2_getattrAll(), which this harness does not exercise. */
#include <sys/msg.h>
void _phoenix_initAttrsStruct(struct _attrAll *attrs, int err)
{
	if (attrs == NULL) {
		return;
	}
	attrs->mode.err = err;  attrs->uid.err = err;   attrs->gid.err = err;
	attrs->size.err = err;  attrs->blocks.err = err; attrs->ioblock.err = err;
	attrs->type.err = err;  attrs->port.err = err;  attrs->pollStatus.err = err;
	attrs->eventMask.err = err; attrs->cTime.err = err; attrs->mTime.err = err;
	attrs->aTime.err = err; attrs->links.err = err; attrs->dev.err = err;
}
