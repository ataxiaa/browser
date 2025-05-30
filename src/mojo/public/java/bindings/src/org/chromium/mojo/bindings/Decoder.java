// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.mojo.bindings;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.mojo.bindings.Interface.Proxy;
import org.chromium.mojo.system.DataPipe;
import org.chromium.mojo.system.Handle;
import org.chromium.mojo.system.InvalidHandle;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.SharedBufferHandle;
import org.chromium.mojo.system.UntypedHandle;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;

/**
 * A Decoder is a helper class for deserializing a mojo struct. It enables deserialization of basic
 * types from a {@link Message} object at a given offset into it's byte buffer.
 */
@NullMarked
public class Decoder {

    /** Helper class to validate the decoded message. */
    static final class Validator {

        /** Minimal value for the next handle to deserialize. */
        private int mMinNextClaimedHandle;

        /** Minimal value of the start of the next memory to claim. */
        private long mMinNextMemory;

        /** The current nesting level when decoding. */
        private long mStackDepth;

        /** The maximal memory accessible. */
        private final long mMaxMemory;

        /** The number of handles in the message. */
        private final long mNumberOfHandles;

        /** The maximum nesting level when decoding. */
        private static final int MAX_RECURSION_DEPTH = 100;

        /** Constructor. */
        Validator(long maxMemory, int numberOfHandles) {
            mMaxMemory = maxMemory;
            mNumberOfHandles = numberOfHandles;
            mStackDepth = 0;
        }

        public void claimHandle(int handle) {
            if (handle < mMinNextClaimedHandle) {
                throw new DeserializationException("Trying to access handle out of order.");
            }
            if (handle >= mNumberOfHandles) {
                throw new DeserializationException("Trying to access non present handle.");
            }
            mMinNextClaimedHandle = handle + 1;
        }

        public void claimMemory(long start, long end) {
            if (start % BindingsHelper.ALIGNMENT != 0) {
                throw new DeserializationException("Incorrect starting alignment: " + start + ".");
            }
            if (start < mMinNextMemory) {
                throw new DeserializationException("Trying to access memory out of order.");
            }
            if (end < start) {
                throw new DeserializationException("Incorrect memory range.");
            }
            if (end > mMaxMemory) {
                throw new DeserializationException("Trying to access out of range memory.");
            }
            mMinNextMemory = BindingsHelper.align(end);
        }

        public void increaseStackDepth() {
            ++mStackDepth;
            if (mStackDepth >= MAX_RECURSION_DEPTH) {
                throw new DeserializationException("Recursion depth limit exceeded.");
            }
        }

        public void decreaseStackDepth() {
            --mStackDepth;
        }
    }

    /** The message to deserialize from. */
    private final Message mMessage;

    /** The base offset in the byte buffer. */
    private final int mBaseOffset;

    /** Validator for the decoded message. */
    private final Validator mValidator;

    /**
     * Constructor.
     *
     * @param message The message to decode.
     */
    public Decoder(Message message) {
        this(message, new Validator(message.getData().limit(), message.getHandles().size()), 0);
    }

    private Decoder(Message message, Validator validator, int baseOffset) {
        mMessage = message;
        mMessage.getData().order(ByteOrder.LITTLE_ENDIAN);
        mBaseOffset = baseOffset;
        mValidator = validator;
    }

    /** Deserializes a {@link DataHeader} at the current position. */
    public DataHeader readDataHeader() {
        // Claim the memory for the header.
        mValidator.claimMemory(mBaseOffset, mBaseOffset + DataHeader.HEADER_SIZE);
        DataHeader result = readDataHeaderAtOffset(0, false);
        // Claim the rest of the memory.
        mValidator.claimMemory(mBaseOffset + DataHeader.HEADER_SIZE, mBaseOffset + result.size);
        return result;
    }

    /** Deserializes a {@link DataHeader} for an union at the given offset. */
    public DataHeader readDataHeaderForUnion(int offset) {
        DataHeader result = readDataHeaderAtOffset(offset, true);
        if (result.size == 0) {
            if (result.elementsOrVersion != 0) {
                throw new DeserializationException(
                        "Unexpected version tag for a null union. Expecting 0, found: "
                                + result.elementsOrVersion);
            }
        } else if (result.size != BindingsHelper.UNION_SIZE) {
            throw new DeserializationException(
                    "Unexpected size of an union. The size must be 0 for a null union, or 16 for "
                            + "a non-null union.");
        }
        return result;
    }

    /**
     * @return a decoder suitable to decode an union defined as the root object of a message.
     */
    public Decoder decoderForSerializedUnion() {
        mValidator.claimMemory(0, BindingsHelper.UNION_SIZE);
        return this;
    }

    /** Deserializes a {@link DataHeader} at the given offset. */
    private DataHeader readDataHeaderAtOffset(int offset, boolean isUnion) {
        int size = readInt(offset + DataHeader.SIZE_OFFSET);
        int elementsOrVersion = readInt(offset + DataHeader.ELEMENTS_OR_VERSION_OFFSET);
        if (size < 0) {
            throw new DeserializationException(
                    "Negative size. Unsigned integers are not valid for java.");
        }
        if (elementsOrVersion < 0 && (!isUnion || elementsOrVersion != -1)) {
            throw new DeserializationException(
                    "Negative elements or version. Unsigned integers are not valid for java.");
        }

        return new DataHeader(size, elementsOrVersion);
    }

    public DataHeader readAndValidateDataHeader(DataHeader[] versionArray) {
        DataHeader header = readDataHeader();
        int maxVersionIndex = versionArray.length - 1;
        if (header.elementsOrVersion <= versionArray[maxVersionIndex].elementsOrVersion) {
            DataHeader referenceHeader = null;
            for (int index = maxVersionIndex; index >= 0; index--) {
                DataHeader dataHeader = versionArray[index];
                if (header.elementsOrVersion >= dataHeader.elementsOrVersion) {
                    referenceHeader = dataHeader;
                    break;
                }
            }
            if (referenceHeader == null || referenceHeader.size != header.size) {
                throw new DeserializationException(
                        "Header doesn't correspond to any known version.");
            }
        } else {
            if (header.size < versionArray[maxVersionIndex].size) {
                throw new DeserializationException(
                        "Message newer than the last known version"
                                + " cannot be shorter than required by the last known version.");
            }
        }
        return header;
    }

    /**
     * Deserializes a {@link DataHeader} at the given offset and checks if it is correct for an
     * array where elements are pointers.
     */
    public DataHeader readDataHeaderForPointerArray(int expectedLength) {
        return readDataHeaderForArray(BindingsHelper.POINTER_SIZE, expectedLength, false);
    }

    /**
     * Deserializes a {@link DataHeader} at the given offset and checks if it is correct for an
     * array where elements are unions.
     */
    public DataHeader readDataHeaderForUnionArray(int expectedLength) {
        return readDataHeaderForArray(BindingsHelper.UNION_SIZE, expectedLength, false);
    }

    /**
     * Deserializes a {@link DataHeader} at the given offset and checks if it is correct for a map.
     */
    public void readDataHeaderForMap() {
        DataHeader si = readDataHeader();
        if (si.size != BindingsHelper.MAP_STRUCT_HEADER.size) {
            throw new DeserializationException("Incorrect header for map. The size is incorrect.");
        }
        if (si.elementsOrVersion != BindingsHelper.MAP_STRUCT_HEADER.elementsOrVersion) {
            throw new DeserializationException(
                    "Incorrect header for map. The version is incorrect.");
        }
    }

    /** Deserializes a byte at the given offset. */
    public byte readByte(int offset) {
        validateBufferSize(offset, 1);
        return mMessage.getData().get(mBaseOffset + offset);
    }

    /** Deserializes a boolean at the given offset, re-using any partially read byte. */
    public boolean readBoolean(int offset, int bit) {
        validateBufferSize(offset, 1);
        return (readByte(offset) & (1 << bit)) != 0;
    }

    /** Deserializes a short at the given offset. */
    public short readShort(int offset) {
        validateBufferSize(offset, 2);
        return mMessage.getData().getShort(mBaseOffset + offset);
    }

    /** Deserializes an int at the given offset. */
    public int readInt(int offset) {
        validateBufferSize(offset, 4);
        return mMessage.getData().getInt(mBaseOffset + offset);
    }

    /** Deserializes a float at the given offset. */
    public float readFloat(int offset) {
        validateBufferSize(offset, 4);
        return mMessage.getData().getFloat(mBaseOffset + offset);
    }

    /** Deserializes a long at the given offset. */
    public long readLong(int offset) {
        validateBufferSize(offset, 8);
        return mMessage.getData().getLong(mBaseOffset + offset);
    }

    /** Deserializes a double at the given offset. */
    public double readDouble(int offset) {
        validateBufferSize(offset, 8);
        return mMessage.getData().getDouble(mBaseOffset + offset);
    }

    /**
     * Deserializes a pointer at the given offset. Returns a Decoder suitable to decode the content
     * of the pointer.
     */
    public @Nullable Decoder readPointer(int offset, boolean nullable) {
        int basePosition = mBaseOffset + offset;
        long pointerOffset = readLong(offset);
        if (pointerOffset == 0) {
            if (!nullable) {
                throw new DeserializationException(
                        "Trying to decode null pointer for a non-nullable type.");
            }
            return null;
        }
        int newPosition = (int) (basePosition + pointerOffset);
        // The method |getDecoderAtPosition| will validate that the pointer address is valid.
        return getDecoderAtPosition(newPosition);
    }

    /** Deserializes an array of boolean at the given offset. */
    public boolean @Nullable [] readBooleans(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForBooleanArray(expectedLength, false);
        byte[] bytes = new byte[(si.elementsOrVersion + 7) / BindingsHelper.ALIGNMENT];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().get(bytes);
        boolean[] result = new boolean[si.elementsOrVersion];
        for (int i = 0; i < bytes.length; ++i) {
            for (int j = 0; j < BindingsHelper.ALIGNMENT; ++j) {
                int booleanIndex = i * BindingsHelper.ALIGNMENT + j;
                if (booleanIndex < result.length) {
                    result[booleanIndex] = (bytes[i] & (1 << j)) != 0;
                }
            }
        }
        return result;
    }

    /** Deserializes an array of Booleans at the given offset. */
    public @Nullable Boolean @Nullable [] readBooleanNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForBooleanArray(expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(1, si.elementsOrVersion, d.mMessage.getData());
        boolean[] values = readBitfield(1, si.elementsOrVersion, d.mMessage.getData());

        @Nullable Boolean[] result = new Boolean[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of bytes at the given offset. */
    public byte @Nullable [] readBytes(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(1, expectedLength, false);
        byte[] result = new byte[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().get(result);
        return result;
    }

    /** Deserializes an array of Bytes at the given offset. */
    public @Nullable Byte @Nullable [] readByteNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(1, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(1, si.elementsOrVersion, d.mMessage.getData());
        byte[] values = new byte[si.elementsOrVersion];
        d.mMessage.getData().get(values);

        @Nullable Byte[] result = new Byte[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of shorts at the given offset. */
    public short @Nullable [] readShorts(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(2, expectedLength, false);
        short[] result = new short[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().asShortBuffer().get(result);
        return result;
    }

    /** Deserializes an array of Shorts at the given offset. */
    public @Nullable Short @Nullable [] readShortNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(2, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(2, si.elementsOrVersion, d.mMessage.getData());
        short[] values = new short[si.elementsOrVersion];
        d.mMessage.getData().asShortBuffer().get(values);

        @Nullable Short[] result = new Short[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of ints at the given offset. */
    public int @Nullable [] readInts(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        int[] result = new int[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().asIntBuffer().get(result);
        return result;
    }

    /** Deserializes an array of Integers at the given offset. */
    public @Nullable Integer @Nullable [] readIntNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(4, si.elementsOrVersion, d.mMessage.getData());
        int[] values = new int[si.elementsOrVersion];
        d.mMessage.getData().asIntBuffer().get(values);

        @Nullable Integer[] result = new Integer[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of floats at the given offset. */
    public float @Nullable [] readFloats(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        float[] result = new float[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().asFloatBuffer().get(result);
        return result;
    }

    /** Deserializes an array of Integers at the given offset. */
    public @Nullable Float @Nullable [] readFloatNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(4, si.elementsOrVersion, d.mMessage.getData());
        float[] values = new float[si.elementsOrVersion];
        d.mMessage.getData().asFloatBuffer().get(values);

        @Nullable Float[] result = new Float[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of longs at the given offset. */
    public long @Nullable [] readLongs(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(8, expectedLength, false);
        long[] result = new long[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().asLongBuffer().get(result);
        return result;
    }

    /** Deserializes an array of Longs at the given offset. */
    public @Nullable Long @Nullable [] readLongNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(8, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(8, si.elementsOrVersion, d.mMessage.getData());
        long[] values = new long[si.elementsOrVersion];
        d.mMessage.getData().asLongBuffer().get(values);

        @Nullable Long[] result = new Long[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an array of doubles at the given offset. */
    public double @Nullable [] readDoubles(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(8, expectedLength, false);
        double[] result = new double[si.elementsOrVersion];
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);
        d.mMessage.getData().asDoubleBuffer().get(result);
        return result;
    }

    /** Deserializes an array of Doubles at the given offset. */
    public @Nullable Double @Nullable [] readDoubleNullables(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(8, expectedLength, true);
        d.mMessage.getData().position(d.mBaseOffset + DataHeader.HEADER_SIZE);

        boolean[] hasValueBitfield = readBitfield(8, si.elementsOrVersion, d.mMessage.getData());
        double[] values = new double[si.elementsOrVersion];
        d.mMessage.getData().asDoubleBuffer().get(values);
        @Nullable Double[] result = new Double[si.elementsOrVersion];
        for (int i = 0; i < si.elementsOrVersion; ++i) {
            if (hasValueBitfield[i]) {
                result[i] = values[i];
            } else {
                result[i] = null;
            }
        }
        return result;
    }

    /** Deserializes an |Handle| at the given offset. */
    public Handle readHandle(int offset, boolean nullable) {
        int index = readInt(offset);
        if (index == -1) {
            if (!nullable) {
                throw new DeserializationException(
                        "Trying to decode an invalid handle for a non-nullable type.");
            }
            return InvalidHandle.INSTANCE;
        }
        mValidator.claimHandle(index);
        return mMessage.getHandles().get(index);
    }

    /** Deserializes an |UntypedHandle| at the given offset. */
    public UntypedHandle readUntypedHandle(int offset, boolean nullable) {
        return readHandle(offset, nullable).toUntypedHandle();
    }

    /** Deserializes a |ConsumerHandle| at the given offset. */
    public DataPipe.ConsumerHandle readConsumerHandle(int offset, boolean nullable) {
        return readUntypedHandle(offset, nullable).toDataPipeConsumerHandle();
    }

    /** Deserializes a |ProducerHandle| at the given offset. */
    public DataPipe.ProducerHandle readProducerHandle(int offset, boolean nullable) {
        return readUntypedHandle(offset, nullable).toDataPipeProducerHandle();
    }

    /** Deserializes a |MessagePipeHandle| at the given offset. */
    public MessagePipeHandle readMessagePipeHandle(int offset, boolean nullable) {
        return readUntypedHandle(offset, nullable).toMessagePipeHandle();
    }

    /** Deserializes a |SharedBufferHandle| at the given offset. */
    public SharedBufferHandle readSharedBufferHandle(int offset, boolean nullable) {
        return readUntypedHandle(offset, nullable).toSharedBufferHandle();
    }

    /**
     * Deserializes an interface at the given offset.
     *
     * @return a proxy to the service.
     */
    public <P extends Proxy> @Nullable P readServiceInterface(
            int offset, boolean nullable, Interface.Manager<?, P> manager) {
        MessagePipeHandle handle = readMessagePipeHandle(offset, nullable);
        if (!handle.isValid()) {
            return null;
        }
        int version = readInt(offset + BindingsHelper.SERIALIZED_HANDLE_SIZE);
        return manager.attachProxy(handle, version);
    }

    /** Deserializes a |InterfaceRequest| at the given offset. */
    public <I extends Interface> @Nullable InterfaceRequest<I> readInterfaceRequest(
            int offset, boolean nullable) {
        MessagePipeHandle handle = readMessagePipeHandle(offset, nullable);
        if (handle == null) {
            return null;
        }
        return new InterfaceRequest<I>(handle);
    }

    /** Deserializes an associated interface at the given offset. Not yet supported. */
    public @Nullable AssociatedInterfaceNotSupported readAssociatedServiceInterfaceNotSupported(
            int offset, boolean nullable) {
        return null;
    }

    /** Deserializes an associated interface request at the given offset. Not yet supported. */
    public @Nullable AssociatedInterfaceRequestNotSupported
            readAssociatedInterfaceRequestNotSupported(int offset, boolean nullable) {
        return null;
    }

    /** Deserializes a string at the given offset. */
    public @Nullable String readString(int offset, boolean nullable) {
        final int arrayNullability = nullable ? BindingsHelper.ARRAY_NULLABLE : 0;
        byte[] bytes = readBytes(offset, arrayNullability, BindingsHelper.UNSPECIFIED_ARRAY_LENGTH);
        if (bytes == null) {
            return null;
        }
        return new String(bytes, Charset.forName("utf8"));
    }

    /** Deserializes an array of |Handle| at the given offset. */
    public Handle @Nullable [] readHandles(int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        Handle[] result = new Handle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |UntypedHandle| at the given offset. */
    public UntypedHandle @Nullable [] readUntypedHandles(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        UntypedHandle[] result = new UntypedHandle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readUntypedHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |ConsumerHandle| at the given offset. */
    public DataPipe.ConsumerHandle @Nullable [] readConsumerHandles(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        DataPipe.ConsumerHandle[] result = new DataPipe.ConsumerHandle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readConsumerHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |ProducerHandle| at the given offset. */
    public DataPipe.ProducerHandle @Nullable [] readProducerHandles(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        DataPipe.ProducerHandle[] result = new DataPipe.ProducerHandle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readProducerHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |MessagePipeHandle| at the given offset. */
    public MessagePipeHandle @Nullable [] readMessagePipeHandles(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        MessagePipeHandle[] result = new MessagePipeHandle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readMessagePipeHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |SharedBufferHandle| at the given offset. */
    public SharedBufferHandle @Nullable [] readSharedBufferHandles(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        SharedBufferHandle[] result = new SharedBufferHandle[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readSharedBufferHandle(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of |ServiceHandle| at the given offset. */
    public <S extends Interface, P extends Proxy> @Nullable S @Nullable [] readServiceInterfaces(
            int offset, int arrayNullability, int expectedLength, Interface.Manager<S, P> manager) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si =
                d.readDataHeaderForArray(
                        BindingsHelper.SERIALIZED_INTERFACE_SIZE, expectedLength, false);
        @Nullable S[] result = manager.buildArray(si.elementsOrVersion);
        for (int i = 0; i < result.length; ++i) {
            // This cast is necessary because java 6 doesn't handle wildcard correctly when using
            // Manager<S, ? extends S>
            @SuppressWarnings("unchecked")
            S value =
                    (S)
                            d.readServiceInterface(
                                    DataHeader.HEADER_SIZE
                                            + BindingsHelper.SERIALIZED_INTERFACE_SIZE * i,
                                    BindingsHelper.isElementNullable(arrayNullability),
                                    manager);
            result[i] = value;
        }
        return result;
    }

    /** Deserializes an array of |InterfaceRequest| at the given offset. */
    public <I extends Interface> @Nullable InterfaceRequest<I> @Nullable [] readInterfaceRequests(
            int offset, int arrayNullability, int expectedLength) {
        Decoder d = readPointer(offset, BindingsHelper.isArrayNullable(arrayNullability));
        if (d == null) {
            return null;
        }
        DataHeader si = d.readDataHeaderForArray(4, expectedLength, false);
        @SuppressWarnings("unchecked")
        @Nullable
        InterfaceRequest<I>[] result = new InterfaceRequest[si.elementsOrVersion];
        for (int i = 0; i < result.length; ++i) {
            result[i] =
                    d.readInterfaceRequest(
                            DataHeader.HEADER_SIZE + BindingsHelper.SERIALIZED_HANDLE_SIZE * i,
                            BindingsHelper.isElementNullable(arrayNullability));
        }
        return result;
    }

    /** Deserializes an array of associated interfaces at the given offset. Not yet supported. */
    public AssociatedInterfaceNotSupported @Nullable [] readAssociatedServiceInterfaceNotSupporteds(
            int offset, int arrayNullability, int expectedLength) {
        return null;
    }

    /**
     * Deserializes an array of associated interface requests at the given offset. Not yet
     * supported.
     */
    public AssociatedInterfaceRequestNotSupported @Nullable []
            readAssociatedInterfaceRequestNotSupporteds(
                    int offset, int arrayNullability, int expectedLength) {
        return null;
    }

    /** Returns a view of this decoder at the offset |offset|. */
    private Decoder getDecoderAtPosition(int offset) {
        return new Decoder(mMessage, mValidator, offset);
    }

    /**
     * Deserializes a {@link DataHeader} at the given offset and checks if it is correct for an
     * array of booleans.
     */
    private DataHeader readDataHeaderForBooleanArray(
            int expectedLength, boolean containsHasValueBitfield) {
        DataHeader dataHeader = readDataHeader();

        int packedBoolSize = (dataHeader.elementsOrVersion + 7) / 8;
        int expectedSize = DataHeader.HEADER_SIZE + packedBoolSize;
        if (containsHasValueBitfield) {
            int hasValueBitfieldSize = packedBoolSize;
            expectedSize += hasValueBitfieldSize;
        }

        if (dataHeader.size < expectedSize) {
            throw new DeserializationException("Array header is incorrect.");
        }
        if (expectedLength != BindingsHelper.UNSPECIFIED_ARRAY_LENGTH
                && dataHeader.elementsOrVersion != expectedLength) {
            throw new DeserializationException(
                    "Incorrect array length. Expected: "
                            + expectedLength
                            + ", but got: "
                            + dataHeader.elementsOrVersion
                            + ".");
        }
        return dataHeader;
    }

    /** Deserializes a {@link DataHeader} of an array at the given offset. */
    private DataHeader readDataHeaderForArray(
            long elementSize, int expectedLength, boolean containsHasValueBitfield) {
        DataHeader dataHeader = readDataHeader();

        long totalElementsSize = elementSize * dataHeader.elementsOrVersion;
        long expectedSize = DataHeader.HEADER_SIZE + totalElementsSize;
        if (containsHasValueBitfield) {
            expectedSize +=
                    BindingsHelper.computeBitfieldSize(elementSize, dataHeader.elementsOrVersion);
        }

        if (dataHeader.size < expectedSize) {
            throw new DeserializationException("Array header is incorrect.");
        }
        if (expectedLength != BindingsHelper.UNSPECIFIED_ARRAY_LENGTH
                && dataHeader.elementsOrVersion != expectedLength) {
            throw new DeserializationException(
                    "Incorrect array length. Expected: "
                            + expectedLength
                            + ", but got: "
                            + dataHeader.elementsOrVersion
                            + ".");
        }
        return dataHeader;
    }

    private static boolean[] readBitfield(int typeSize, int numElements, ByteBuffer buffer) {
        boolean[] bitfield = new boolean[numElements];

        byte[] b = new byte[BindingsHelper.computeBitfieldSize(typeSize, numElements)];
        buffer.get(b);

        for (int i = 0; i < numElements; ++i) {
            int idx = i / 8;
            int mask = 1 << (i % 8);
            bitfield[i] = (b[idx] & mask) != 0;
        }
        return bitfield;
    }

    private void validateBufferSize(int offset, int size) {
        if (mMessage.getData().limit() < offset + size) {
            throw new DeserializationException("Buffer is smaller than expected.");
        }
    }

    public void increaseStackDepth() {
        mValidator.increaseStackDepth();
    }

    public void decreaseStackDepth() {
        mValidator.decreaseStackDepth();
    }
}
