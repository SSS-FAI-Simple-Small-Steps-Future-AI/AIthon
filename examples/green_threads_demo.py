"""
PyVM Green Threads Example
Demonstrates:
- Actor-based concurrency
- Independent memory spaces
- Message passing
- Fault isolation
- No shared memory
"""

async def worker(worker_id, num_tasks):
    """
    Each worker is an independent green thread with:
    - Private memory (2MB heap)
    - Independent garbage collector
    - Crash isolation
    - Message-based communication only
    """
    print(f"Worker {worker_id} started with {num_tasks} tasks")

    results = []
    for i in range(num_tasks):
        # Simulate work
        result = await process_task(worker_id, i)
        results.append(result)

        # Check if we should yield to other green threads
        if i % 10 == 0:
            await yield_control()

    print(f"Worker {worker_id} completed {num_tasks} tasks")
    return results

async def process_task(worker_id, task_id):
    """
    Process a single task.
    This runs in isolated memory - no shared state!
    """
    # Simulate computation
    value = task_id * 2 + worker_id
    return value

async def supervisor():
    """
    Supervisor manages worker green threads.
    If a worker crashes, it doesn't affect others.
    """
    num_workers = 10
    tasks_per_worker = 100

    print(f"Spawning {num_workers} workers...")

    # Spawn worker green threads
    workers = []
    for i in range(num_workers):
        w = await spawn_green_thread(worker, i, tasks_per_worker)
        workers.append(w)

    print(f"All workers spawned. Waiting for completion...")

    # Wait for all workers
    all_results = []
    for w in workers:
        result = await w
        all_results.extend(result)

    print(f"All workers completed. Total results: {len(all_results)}")
    return all_results

async def fault_tolerant_worker(worker_id):
    """
    Example of fault isolation.
    If this worker crashes, other workers continue running.
    """
    try:
        if worker_id == 5:
            # Simulate crash
            raise Exception(f"Worker {worker_id} simulated crash!")

        # Normal work
        result = await do_work(worker_id)
        return result

    except Exception as e:
        print(f"Worker {worker_id} crashed: {e}")
        # Supervisor will restart this worker if configured
        return None

async def do_work(worker_id):
    """Simulate some work"""
    return worker_id * 100

def main():
    """
    Main entry point.
    PyVM will:
    1. Validate this is the only main() function
    2. Check Python 3.12 syntax
    3. Generate LLVM IR
    4. Create optimized machine code
    5. Link with green thread runtime
    """
    print("╔════════════════════════════════════════════╗")
    print("║    PyVM Green Threads Example              ║")
    print("║    Actor-Based Concurrency                 ║")
    print("╚════════════════════════════════════════════╝")
    print()

    print("Runtime Configuration:")
    print("  • Green Threads: M:N threading model")
    print("  • Memory: Isolated 2MB heap per thread")
    print("  • GC: Per-thread garbage collection")
    print("  • Communication: Message passing only")
    print("  • Fault Tolerance: Crash isolation")
    print()

    # Run the supervisor
    results = await supervisor()

    print()
    print("═" * 48)
    print(f"Final Results: {len(results)} values computed")
    print("All workers completed successfully!")
    print("═" * 48)

# PyVM will use this as the entry point
if __name__ == "__main__":
    main()